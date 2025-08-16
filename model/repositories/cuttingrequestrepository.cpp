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

bool CuttingRequestRepository::tryLoadFromSettings(CuttingPlanRequestRegistry& registry) {
    QString fn = SettingsManager::instance().cuttingPlanFileName();
    const QString filePath = FileNameHelper::instance().getCuttingPlanFilePath(fn);

    if (filePath.isEmpty() || !QFile::exists(filePath)) {
        qWarning() << "⛔ Nincs elérhető vágási terv fájl: " << filePath;
        return false;
    }

    return loadFromFile(registry, filePath);
}

// ha a beolvasás sikeres, törli a registry-t és feltölti az új adatokkal
bool CuttingRequestRepository::loadFromFile(CuttingPlanRequestRegistry& registry, const QString& filePath) {
    if (filePath.isEmpty() || !QFile::exists(filePath)) {
        qWarning() << "❌ Nem található vagy üres fájlnév: " << filePath;
        return false;
    }

    QVector<Cutting::Plan::Request> requests = loadFromCsv_private(filePath);

    lastFileWasEffectivelyEmpty = requests.isEmpty() && FileHelper::isCsvWithOnlyHeader(filePath);
    if (lastFileWasEffectivelyEmpty) {
        zInfo("ℹ️ Cutting plan file only contains header — valid state, no requests found");
        //registry.setPersist(false);
        //registry.clearAll();
        registry.setData({}); // Clear the registry);
        //registry.setPersist(true);
        return true;
    }

    if (requests.isEmpty()) {
        zWarning("⚠️ Cutting plan file is invalid or improperly formatted");
        return false;
    }

    //registry.setPersist(false);
    // registry.clearAll();
    // for (const auto &req : std::as_const(requests))
    //     registry.registerRequest(req);

    registry.setData(requests);

    //registry.setPersist(true);
    return true;
}

QVector<Cutting::Plan::Request>
CuttingRequestRepository::loadFromCsv_private(const QString& filepath) {
    // QFile file(filepath);
    // if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    //     qWarning() << "❌ Nem sikerült megnyitni a fájlt:" << filepath;
    //     return {};
    // }

    // QTextStream in(&file);
    // in.setEncoding(QStringConverter::Utf8);

    //const auto rows = FileHelper::parseCSV(&in, ';');
    //return CsvImporter::processCsvRows<Cutting::Plan::Request>(rows, convertRowToCuttingRequest);
    return CsvReader::readAndConvert<Cutting::Plan::Request>(filepath, convertRowToCuttingRequest, true);
}


std::optional<CuttingRequestRepository::CuttingRequestRow>
CuttingRequestRepository::convertRowToCuttingRequestRow(const QVector<QString>& parts, int lineIndex) {
    if (parts.size() < 5) {
        qWarning() << QString("⚠️ Sor %1: kevés adat").arg(lineIndex);
        return std::nullopt;
    }

    CuttingRequestRow row;
    row.barcode           = parts[0].trimmed();
    row.requiredLength    = parts[1].trimmed().toInt();
    row.quantity          = parts[2].trimmed().toInt();
    row.ownerName         = parts[3].trimmed();
    row.externalReference = parts[4].trimmed();

    if (row.barcode.isEmpty() || row.requiredLength <= 0 || row.quantity <= 0) {
        qWarning() << QString("⚠️ Sor %1: érvénytelen mező").arg(lineIndex);
        return std::nullopt;
    }

    return row;
}

std::optional<Cutting::Plan::Request>
CuttingRequestRepository::buildCuttingRequestFromRow(const CuttingRequestRow& row, int lineIndex) {
    const auto* mat = MaterialRegistry::instance().findByBarcode(row.barcode);
    if (!mat) {
        qWarning() << QString("⚠️ Sor %1: ismeretlen barcode '%2'")
                          .arg(lineIndex).arg(row.barcode);
        return std::nullopt;
    }

    Cutting::Plan::Request req;
    req.materialId        = mat->id;
    req.requiredLength    = row.requiredLength;
    req.quantity          = row.quantity;
    req.ownerName         = row.ownerName;
    req.externalReference = row.externalReference;

    if (!req.isValid()) {
        qWarning() << QString("⚠️ Sor %1: érvénytelen CuttingRequest").arg(lineIndex);
        return std::nullopt;
    }

    return req;
}

std::optional<Cutting::Plan::Request>
CuttingRequestRepository::convertRowToCuttingRequest(const QVector<QString>& parts, int lineIndex) {
    const auto rowOpt = convertRowToCuttingRequestRow(parts, lineIndex);
    if (!rowOpt.has_value()) return std::nullopt;

    return buildCuttingRequestFromRow(rowOpt.value(), lineIndex);
}



bool CuttingRequestRepository::saveToFile(const CuttingPlanRequestRegistry& registry, const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "❌ Nem sikerült megnyitni a fájlt írásra:" << filePath;
        return false;
    }

    QTextStream out(&file);

    // 📋 CSV fejléc
    out << "materialBarCode;requiredLength;quantity;ownerName;externalReference\n";

    for (const Cutting::Plan::Request& req : registry.readAll()) {
        const auto* material = MaterialRegistry::instance().findById(req.materialId);
        if (!material) {
            qWarning() << "⚠️ Anyag nem található mentéskor:" << req.materialId.toString();
            continue;
        }

        out << material->barcode << ";"
            << req.requiredLength << ";"
            << req.quantity << ";"
            << "\"" << req.ownerName << "\";"
            << "\"" << req.externalReference << "\"\n";
    }

    file.close();
    return true;
}


// std::optional<CuttingRequest> CuttingRequestRepository::convertRowToRequest(const QVector<QString>& parts, int lineIndex) {
//     if (parts.size() < 5) {
//         qWarning() << QString("⚠️ Sor %1 hibás (kevés mező):").arg(lineIndex) << parts;
//         return std::nullopt;
//     }

//     const QString barcode = parts[0].trimmed();
//     const auto* mat = MaterialRegistry::instance().findByBarcode(barcode);
//     if (!mat) {
//         qWarning() << QString("⚠️ Ismeretlen anyag sor %1-ben:").arg(lineIndex) << barcode;
//         return std::nullopt;
//     }

//     CuttingRequest req;
//     req.materialId         = mat->id;
//     req.requiredLength     = parts[1].trimmed().toInt();
//     req.quantity           = parts[2].trimmed().toInt();
//     req.ownerName          = parts[3].trimmed();
//     req.externalReference  = parts[4].trimmed();

//     if (!req.isValid()) {
//         qWarning() << QString("⚠️ Érvénytelen vágási igény sor %1-ben:").arg(lineIndex) << req.toString();
//         return std::nullopt;
//     }

//     return req;
// }
