#include "cuttingrequestrepository.h"
#include "../cutting/plan/request.h"
#include "../registries/cuttingplanrequestregistry.h"
#include "common/logger.h"
#include <QFile>
#include <QTextStream>
#include <QDebug>
#include <common/filenamehelper.h>
#include <common/settingsmanager.h>
#include <model/registries/materialregistry.h>
#include <common/filehelper.h>
#include <common/csvimporter.h>


// vágási terv nélkül tudunk létezni - mi hozzuk létre őket és ez nem törzsadat
CuttingPlanLoadResult CuttingRequestRepository::tryLoadFromSettings(CuttingPlanRequestRegistry& registry) {
    QString fn = SettingsManager::instance().cuttingPlanFileName();

    if (fn.isEmpty())
        return CuttingPlanLoadResult::NoFileConfigured;

    const QString filePath = FileNameHelper::instance().getCuttingPlanFilePath(fn);


    if (!QFile::exists(filePath))
        return CuttingPlanLoadResult::FileMissing;

    bool ok =  loadFromFile(registry, filePath);
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
CuttingRequestRepository::loadFromCsv_private(CsvReader::FileContext& ctx) {
    return CsvReader::readAndConvert<Cutting::Plan::Request>(ctx, convertRowToCuttingRequest, true);
}

std::optional<CuttingRequestRepository::CuttingRequestRow>
CuttingRequestRepository::convertRowToCuttingRequestRow(const QVector<QString>& parts, CsvReader::FileContext& ctx) {
    if (parts.size() < 11) {
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
    row.requiredColorName = parts[7].trimmed();
    row.barcode           = parts[8].trimmed();
    row.relevantDimStr    = parts[9].trimmed();
    row.isMeasurementNeeded = (parts[10].trimmed().toLower() == "true");

    if (row.barcode.isEmpty() || row.requiredLength <= 0 || row.quantity <= 0) {
        QString msg = L("⚠️ Érvénytelen mező");
        ctx.addError(ctx.currentLineNumber(), msg);
        return std::nullopt;
    }

    return row;
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
    req.requiredColorName = row.requiredColorName;
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

    return req;
}


std::optional<Cutting::Plan::Request>
CuttingRequestRepository::convertRowToCuttingRequest(const QVector<QString>& parts, CsvReader::FileContext& ctx) {
    const auto rowOpt = convertRowToCuttingRequestRow(parts, ctx);
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
    out << "externalReference;ownerName;fullWidth_mm;fullHeight_mm;requiredLength;tolerance;quantity;requiredColorName;materialBarCode;relevantDim;isMeasurementNeeded\n";

    for (const Cutting::Plan::Request& req : registry.readAll()) {
        const auto* material = MaterialRegistry::instance().findById(req.materialId);
        if (!material) {
            qWarning() << "⚠️ Anyag nem található mentéskor:" << req.materialId.toString();
            continue;
        }

        QString toleranceStr = req.requiredTolerance.has_value()
                                   ? req.requiredTolerance->toCsvString()
                                   : "";

        out << "\"" << req.externalReference << "\";"
            << "\"" << req.ownerName << "\";"
            << req.fullWidth_mm << ";"
            << req.fullHeight_mm << ";"
            << req.requiredLength << ";"
            << toleranceStr << ";"
            << req.quantity << ";"
            << "\"" << req.requiredColorName << "\";"
            << material->barcode << ";"
            << (req.relevantDim == RelevantDimension::Width ? "Width" : "Height") << ";"
            << (req.isMeasurementNeeded ? "true" : "false") << "\n";
    }


    file.close();
    return true;
}


