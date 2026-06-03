#include "cuttingrequestrepository.h"
#include "../cutting/plan/request.h"
#include "../registries/cuttingplanrequestregistry.h"
#include "../../common/logger.h"
#include <QFile>
#include <QTextStream>
#include <QDebug>
#include "../../common/filenamehelper.h"
#include "../../common/settingsmanager.h"
#include "materials/registry/material_registry.h"
#include "../../common/filehelper.h"
#include "../../common/csvimporter.h"


// vágási terv nélkül tudunk létezni - mi hozzuk létre őket és ez nem törzsadat

CuttingPlanLoadResult CuttingRequestRepository::tryLoadFromSettings(
    CuttingPlanRequestRegistry &registry) {
    QString fn = SettingsManager::instance().cuttingPlanFileName();

    if (fn.isEmpty())
        return CuttingPlanLoadResult::NoFileConfigured;

    const QString filePath =
        FileNameHelper::instance().getCuttingPlanFilePath(fn);

    if (!QFile::exists(filePath))
        return CuttingPlanLoadResult::FileMissing;

    bool ok = loadFromFile(registry, filePath);
    return ok ? CuttingPlanLoadResult::Success : CuttingPlanLoadResult::LoadError;
}

// ha a beolvasás sikeres, törli a registry-t és feltölti az új adatokkal
bool CuttingRequestRepository::loadFromFile(CuttingPlanRequestRegistry& registry, const QString& filePath) {
    if (filePath.isEmpty() || !QFile::exists(filePath)) {
        qWarning() << "❌ Nem található vagy üres fájlnév: " << filePath;
        return false;
    }

    CsvReader::FileContext ctx(filePath);
    QVector<Cutting::Plan::Request> requests = loadFromCsv_private(ctx);

    // 🔍 Hibák loggolása
    if (ctx.hasErrors()) {
        zWarning(QString("⚠️ Hibák a vágási terv importálása során (%1 sor):").arg(ctx.errorsSize()));
        zWarning(ctx.toString());
    }

    lastFileWasEffectivelyEmpty = requests.isEmpty() && FileHelper::isCsvWithOnlyHeader(filePath);
    if (lastFileWasEffectivelyEmpty) {
        zInfo("ℹ️ Cutting plan file only contains header — valid state, no requests found");        //registry.setPersist(false);
        registry.setDataEphemeral({}); // Clear the registry);
        return true;
    }

    if (requests.isEmpty()) {
        zWarning("❌ A cutting plan fájl hibás vagy nem tartalmaz érvényes adatot.");
        return false;
    }

    registry.setDataEphemeral(requests);

    zInfo(QString("✅ %1 vágási igény sikeresen importálva a fájlból: %2").arg(requests.size()).arg(filePath));

    //registry.setPersist(true);
    return true;
}

QVector<Cutting::Plan::Request>
CuttingRequestRepository::loadFromCsv_private(CsvReader::FileContext& ctx)
{
//    return CsvReader::readAndConvert<Cutting::Plan::Request>(ctx, convertRowToCuttingRequest, true);
    CSVVersion ver = detectCsvVersion(ctx.filepath());

    switch (ver) {
    case CSVVersion::V1_OldHandlerSide:
        return CsvReader::readAndConvert<Cutting::Plan::Request>(ctx, convertRowToCuttingRequest_V1, true);

    case CSVVersion::V2_LeftRightSubtype:
        return CsvReader::readAndConvert<Cutting::Plan::Request>(ctx, convertRowToCuttingRequest_V2, true);

    case CSVVersion::V3_WithDueDate:
        return CsvReader::readAndConvert<Cutting::Plan::Request>(ctx, convertRowToCuttingRequest_V3, true);

    default:
        ctx.addError(0, "Ismeretlen CSV formátum – fejléc nem értelmezhető");
        return {};
    }

}

std::optional<CuttingRequestRepository::CuttingRequestRow>
CuttingRequestRepository::convertRowToCuttingRequestRow_V1(const QVector<QString>& parts, CsvReader::FileContext& ctx) {
    if (parts.size() < 12) {
        QString msg = L("⚠️ Kevés adat");
        ctx.addError(ctx.currentLineNumber(), msg);
        return std::nullopt;
    }

    CuttingRequestRow row;
    row.externalReference = parts[0].trimmed();
    row.ownerName         = parts[1].trimmed();
    row.fullWidth_mm      = parts[2].trimmed().toInt();
    row.fullHeight_mm     = parts[3].trimmed().toInt();
    row.requiredLength    = parts[4].trimmed().toInt();
    row.toleranceStr      = parts[5].trimmed();
    row.quantity          = parts[6].trimmed().toInt();
    row.handlerSide = parts[7].trimmed();
    row.requiredColorName = parts[8].trimmed();
    row.barcode           = parts[9].trimmed();
    row.relevantDimStr    = parts[10].trimmed();
    row.isMeasurementNeeded = (parts[11].trimmed().toLower() == "true");
//V2
    auto side = HandlerSideUtils::parse(row.handlerSide);
    row.leftCount         = side == HandlerSide::Left ? row.quantity : 0;
    row.rightCount        = side == HandlerSide::Right ? row.quantity : 0;
    row.subtypeStr        = "none";

    if (row.barcode.isEmpty() || row.requiredLength <= 0 || row.quantity <= 0) {
        QString msg = L("⚠️ Érvénytelen mező");
        ctx.addError(ctx.currentLineNumber(), msg);
        return std::nullopt;
    }

    return row;
}

std::optional<CuttingRequestRepository::CuttingRequestRow>
CuttingRequestRepository::convertRowToCuttingRequestRow_V2(const QVector<QString>& parts, CsvReader::FileContext& ctx)
{
    if (parts.size() < 14)
        return std::nullopt;

    CuttingRequestRepository::CuttingRequestRow row;
    row.externalReference = parts[0].trimmed();
    row.ownerName         = parts[1].trimmed();
    row.fullWidth_mm      = parts[2].trimmed().toInt();
    row.fullHeight_mm     = parts[3].trimmed().toInt();
    row.requiredLength    = parts[4].trimmed().toInt();
    row.toleranceStr      = parts[5].trimmed();
    row.quantity          = parts[6].trimmed().toInt();

    row.leftCount         = parts[7].trimmed().toInt();
    row.rightCount        = parts[8].trimmed().toInt();
    row.subtypeStr        = parts[9].trimmed();

    row.requiredColorName = parts[10].trimmed();
    row.barcode           = parts[11].trimmed();
    row.relevantDimStr    = parts[12].trimmed();
    row.isMeasurementNeeded = (parts[13].trimmed().toLower() == "true");

    return row;
}

std::optional<CuttingRequestRepository::CuttingRequestRow>
CuttingRequestRepository::convertRowToCuttingRequestRow_V3(const QVector<QString>& parts,
                                                           CsvReader::FileContext& ctx)
{
    if (parts.size() < 15) {
        ctx.addError(ctx.currentLineNumber(), L("⚠️ Kevés adat (V3)"));
        return std::nullopt;
    }

    CuttingRequestRow row;
    row.externalReference = parts[0].trimmed();
    row.ownerName         = parts[1].trimmed();
    row.fullWidth_mm      = parts[2].trimmed().toInt();
    row.fullHeight_mm     = parts[3].trimmed().toInt();
    row.requiredLength    = parts[4].trimmed().toInt();
    row.toleranceStr      = parts[5].trimmed();
    row.quantity          = parts[6].trimmed().toInt();

    row.leftCount         = parts[7].trimmed().toInt();
    row.rightCount        = parts[8].trimmed().toInt();
    row.subtypeStr        = parts[9].trimmed();

    row.requiredColorName = parts[10].trimmed();
    row.barcode           = parts[11].trimmed();
    row.relevantDimStr    = parts[12].trimmed();
    row.isMeasurementNeeded = (parts[13].trimmed().toLower() == "true");

    // ⭐ dueDate (YYYY-MM-DD)
    QString dueStr = parts[14].trimmed();
    QDate d = QDate::fromString(dueStr, "yyyy-MM-dd");
    row.dueDate = d.isValid() ? d : QDate::currentDate();

    return row;
}

std::optional<Cutting::Plan::Request>
CuttingRequestRepository::convertRowToCuttingRequest_V3(const QVector<QString>& parts,
                                                        CsvReader::FileContext& ctx)
{
    const auto rowOpt = convertRowToCuttingRequestRow_V3(parts, ctx);
    if (!rowOpt.has_value()) return std::nullopt;

    return buildCuttingRequestFromRow(rowOpt.value(), ctx);
}

std::optional<Cutting::Plan::Request>
CuttingRequestRepository::buildCuttingRequestFromRow(const CuttingRequestRow& row, CsvReader::FileContext& ctx) {
    const auto* mat = MaterialRegistry::instance().findByBarcode(row.barcode);
    if (!mat) {
        QString msg = L("⚠️ Ismeretlen barcode '%1'").arg(row.barcode);
        ctx.addError(ctx.currentLineNumber(), msg);
        return std::nullopt;
    }

    Cutting::Plan::Request req;
    req.materialId        = mat->id;
    req.externalReference = row.externalReference;
    req.ownerName         = row.ownerName;
    req.fullWidth_mm      = row.fullWidth_mm;
    req.fullHeight_mm     = row.fullHeight_mm;
    req.requiredLength    = row.requiredLength;
    req.quantity          = row.quantity;
    req.leftCount  = row.leftCount;
    req.rightCount = row.rightCount;
    req.subtype    = SubtypeUtils::parse(row.subtypeStr);
    req.isMeasurementNeeded = row.isMeasurementNeeded;

    // 🔍 Tűrés beolvasása
    if (!row.toleranceStr.isEmpty()) {
        auto tolOpt = Tolerance::fromString(row.toleranceStr);
        if (tolOpt.has_value()) {
            req.requiredTolerance = tolOpt;
        } else {
            QString msg = L("⚠️ Érvénytelen tűrés formátum: '%1'").arg(row.toleranceStr);
            ctx.addError(ctx.currentLineNumber(), msg);
        }
    }

    // 🔍 RelevantDimension konverzió
    if (row.relevantDimStr.compare("Width", Qt::CaseInsensitive) == 0) {
        req.relevantDim = RelevantDimension::Width;
    } else if (row.relevantDimStr.compare("Height", Qt::CaseInsensitive) == 0) {
        req.relevantDim = RelevantDimension::Height;
    } else {
        QString msg = L("⚠️ Érvénytelen relevantDim érték: '%1'").arg(row.relevantDimStr);
        ctx.addError(ctx.currentLineNumber(), msg);
    }

    if (!req.isValid()) {
        QString msg = L("⚠️ Érvénytelen CuttingRequest");
        ctx.addError(ctx.currentLineNumber(), msg);
        return std::nullopt;
    }

    // 🎨 Szín hozzárendelés – RAL, HEX vagy üres
    if (!row.requiredColorName.isEmpty()) {
        req.requiredColor = NamedColor(row.requiredColorName);
        if (!req.requiredColor.isValid()) {
            QString msg = L("⚠️ Ismeretlen színformátum: %1").arg( row.requiredColorName);
            ctx.addError(ctx.currentLineNumber(), msg);
        }
    } else {
        req.requiredColor = NamedColor(); // nincs festve
    }

    req.dueDate = row.dueDate.isValid()
                      ? row.dueDate
                      : QDate::currentDate();

    return req;
}


std::optional<Cutting::Plan::Request>
CuttingRequestRepository::convertRowToCuttingRequest_V1(const QVector<QString>& parts, CsvReader::FileContext& ctx) {
    const auto rowOpt = convertRowToCuttingRequestRow_V1(parts, ctx);
    if (!rowOpt.has_value()) return std::nullopt;

    return buildCuttingRequestFromRow(rowOpt.value(), ctx);
}

std::optional<Cutting::Plan::Request>
CuttingRequestRepository::convertRowToCuttingRequest_V2(const QVector<QString>& parts, CsvReader::FileContext& ctx) {
    const auto rowOpt = convertRowToCuttingRequestRow_V2(parts, ctx);
    if (!rowOpt.has_value()) return std::nullopt;

    return buildCuttingRequestFromRow(rowOpt.value(), ctx);
}



bool CuttingRequestRepository::saveToFile(const CuttingPlanRequestRegistry& registry, const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "❌ Nem sikerült megnyitni a fájlt írásra:" << filePath;
        return false;
    }

    QTextStream out(&file);

    // 📋 CSV fejléc
    // régi fejléc (V1):
    // externalReference;ownerName;fullWidth_mm;fullHeight_mm;requiredLength;tolerance;quantity;handlerSide;requiredColorName;materialBarCode;relevantDim;isMeasurementNeeded

    // új fejléc (V2):
    out << "externalReference;ownerName;fullWidth_mm;fullHeight_mm;requiredLength;tolerance;quantity;leftCount;rightCount;subtype;requiredColorName;materialBarCode;relevantDim;isMeasurementNeeded;dueDate\n";

    for (const Cutting::Plan::Request& req : registry.readAll()) {
        const auto* material = MaterialRegistry::instance().findById(req.materialId);
        if (!material) {
            qWarning() << "⚠️ Anyag nem található mentéskor:" << req.materialId.toString();
            continue;
        }

        QString toleranceStr = req.requiredTolerance.has_value()
                                   ? req.requiredTolerance->toCsvString()
                                   : "";

        QString dueStr = req.dueDate.toString("yyyy-MM-dd");

        out << "\"" << req.externalReference << "\";"
            << "\"" << req.ownerName << "\";"
            << req.fullWidth_mm << ";"
            << req.fullHeight_mm << ";"
            << req.requiredLength << ";"
            << toleranceStr << ";"
            << req.quantity << ";"
            << req.leftCount << ";"
            << req.rightCount << ";"
            << SubtypeUtils::toString_CSV(req.subtype) << ";"
            << "\"" << req.requiredColor.name() << "\";"
            << material->barcode << ";"
            << (req.relevantDim == RelevantDimension::Width ? "Width" : "Height") << ";"
            << (req.isMeasurementNeeded ? "true" : "false") << ";"
            << dueStr
            << "\n";

    }


    file.close();
    return true;
}

CuttingRequestRepository::CSVVersion CuttingRequestRepository::detectCsvVersion(const QString& filepath)
{
    QFile f(filepath);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
        return CuttingRequestRepository::CSVVersion::Unknown;

    QTextStream in(&f);
    in.setEncoding(QStringConverter::Utf8);

    QString headerLine = in.readLine().trimmed();
    if (headerLine.isEmpty())
        return CuttingRequestRepository::CSVVersion::Unknown;

    QStringList cols = headerLine.split(';', Qt::KeepEmptyParts);

    bool hasHandlerSide = cols.contains("handlerSide", Qt::CaseInsensitive);
    bool hasLeft        = cols.contains("leftCount", Qt::CaseInsensitive);
    bool hasRight       = cols.contains("rightCount", Qt::CaseInsensitive);
    bool hasSubtype     = cols.contains("subtype", Qt::CaseInsensitive);
    bool hasDueDate     = cols.contains("dueDate", Qt::CaseInsensitive);

    if (hasHandlerSide && !hasLeft)
        return CSVVersion::V1_OldHandlerSide;

    // ⭐ V3: ugyanaz mint V2, de dueDate is van
    if (hasLeft && hasRight && hasSubtype && hasDueDate)
        return CSVVersion::V3_WithDueDate;

    // ⭐ V2: nincs dueDate
    if (hasLeft && hasRight && hasSubtype)
        return CSVVersion::V2_LeftRightSubtype;

    return CuttingRequestRepository::CSVVersion::Unknown;
}

