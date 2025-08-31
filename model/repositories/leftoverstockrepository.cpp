#include "leftoverstockrepository.h"
#include <QFile>
#include <QTextStream>
#include <QDebug>
#include "common/filenamehelper.h"
#include "service/cutting/result/leftoversourceutils.h"
#include <model/registries/materialregistry.h>
#include <model/registries/storageregistry.h>
#include <common/filehelper.h>
#include <common/csvimporter.h>

bool LeftoverStockRepository::loadFromCSV(LeftoverStockRegistry& registry) {
    auto& helper = FileNameHelper::instance();
    if (!helper.isInited()) return false;

    QString path = helper.getLeftoversCsvFile();
    if (path.isEmpty()) {
        qWarning("Nincs elérhető leftovers.csv fájl");
        return false;
    }

    QVector<LeftoverStockEntry> entries = loadFromCSV_private(path);
    if (entries.isEmpty()) {
        qWarning("A leftovers.csv fájl üres vagy hibás sorokat tartalmaz.");
        return false;
    }

    // registry.setPersist(false);
    // registry.clearAll(); // 🔄 Korábbi készlet törlése
    // for (const auto& entry : std::as_const(entries))
    //     registry.registerEntry(entry);
    // registry.setPersist(true);

    registry.setData(entries); // 🔧 Itt történik a készletregisztráció

    return true;
}

// QVector<ReusableStockEntry> ReusableStockRepository::loadFromCSV_private(const QString& filepath) {
//     QVector<ReusableStockEntry> result;

//     QFile file(filepath);
//     if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
//         qWarning() << "❌ Nem sikerült megnyitni a leftovers fájlt:" << filepath;
//         return result;
//     }

//     QTextStream in(&file);
//     in.setEncoding(QStringConverter::Utf8);

//     const QList<QVector<QString>> rows = FileHelper::parseCSV(&in, ';');
//     if (rows.isEmpty()) {
//         qWarning() << "⚠️ A leftovers.csv fájl üres vagy hibás.";
//         return result;
//     }

//     for (int i = 0; i < rows.size(); ++i) {
//         // ✅ Fejléc sor kihagyása
//         if (i == 0) continue;

//         const QVector<QString>& parts = rows[i];
//         auto maybeEntry = convertRowToReusableEntry(parts, i + 1);
//         if (maybeEntry.has_value())
//             result.append(maybeEntry.value());
//     }

//     return result;
// }

QVector<LeftoverStockEntry>
LeftoverStockRepository::loadFromCSV_private(const QString& filepath) {
    // QFile file(filepath);
    // if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    //     qWarning() << "❌ Nem sikerült megnyitni a leftovers fájlt:" << filepath;
    //     return {};
    // }

    // QTextStream in(&file);
    // in.setEncoding(QStringConverter::Utf8);
    // const auto rows = FileHelper::parseCSV(&in, ';');

    //return CsvImporter::processCsvRows<LeftoverStockEntry>(rows, convertRowToReusableEntry);
    return CsvReader::readAndConvert<LeftoverStockEntry>(filepath, convertRowToReusableEntry, true);
}


std::optional<LeftoverStockRepository::ReusableStockRow>
LeftoverStockRepository::convertRowToReusableRow(const QVector<QString>& parts, CsvReader::RowContext& ctx) {
    if (parts.size() < 6) {
        qWarning() << QString("⚠️ Sor %1: kevés oszlop").arg(ctx.lineIndex);
        return std::nullopt;
    }

    ReusableStockRow row;
    row.materialBarcode = parts[0].trimmed();

    bool okLength = false;
    row.availableLength_mm = parts[1].trimmed().toInt(&okLength);
    if (row.materialBarcode.isEmpty() || !okLength || row.availableLength_mm <= 0) {
        qWarning() << QString("⚠️ Sor %1: hibás barcode vagy hossz").arg(ctx.lineIndex);
        return std::nullopt;
    }

    // const QString sourceStr = parts[2].trimmed();
    // if (sourceStr == "Optimization")
    //     row.source = LeftoverSource::Optimization;
    // else if (sourceStr == "Manual")
    //     row.source = LeftoverSource::Manual;
    // else {
    //     qWarning() << QString("⚠️ Sor %1: ismeretlen forrástípus").arg(lineIndex);
    //     return std::nullopt;
    // }

    row.source= LeftoverSourceUtils::fromString(parts[2].trimmed());
    if (row.source == Cutting::Result::LeftoverSource::Undefined) {
        qWarning() << QString("⚠️ Sor %1: ismeretlen forrástípus").arg(ctx.lineIndex);
        return std::nullopt;
    }

    if (row.source == Cutting::Result::LeftoverSource::Optimization && parts.size() > 3) {
        bool okOpt = false;
        const int optId = parts[3].trimmed().toInt(&okOpt);
        if (okOpt)
            row.optimizationId = optId;
        else {
            qWarning() << QString("⚠️ Sor %1: hibás optimalizáció ID").arg(ctx.lineIndex);
            return std::nullopt;
        }
    }

    row.barcode = parts[4].trimmed();
    if (row.barcode.isEmpty()) {
        qWarning() << QString("⚠️ Sor %1: hiányzó egyedi barcode").arg(ctx.lineIndex);
        return std::nullopt;
    }

    row.storageBarcode = parts[5].trimmed();
    if (row.storageBarcode.isEmpty()) {
        qWarning() << QString("⚠️ Sor %1: hiányzó tároló barcode").arg(ctx.lineIndex);
        return std::nullopt;
    }

    return row;
}

std::optional<LeftoverStockEntry>
LeftoverStockRepository::buildReusableEntryFromRow(const ReusableStockRow& row, CsvReader::RowContext& ctx) {
    const auto* mat = MaterialRegistry::instance().findByBarcode(row.materialBarcode);
    if (!mat) {
        qWarning() << QString("⚠️ Sor %1: ismeretlen anyag barcode '%2'").arg(ctx.lineIndex).arg(row.materialBarcode);
        return std::nullopt;
    }

    const auto* storage = StorageRegistry::instance().findByBarcode(row.storageBarcode);
    if (!storage) {
        qWarning() << QString("⚠️ Sor %1: ismeretlen tároló barcode '%2'")
                          .arg(ctx.lineIndex).arg(row.storageBarcode);
        return std::nullopt;
    }

    LeftoverStockEntry entry;
    entry.materialId         = mat->id;
    entry.availableLength_mm = row.availableLength_mm;
    entry.barcode            = row.barcode;
    entry.source             = row.source;
    entry.optimizationId     = row.optimizationId;
    entry.storageId = storage->id;

    return entry;
}

std::optional<LeftoverStockEntry>
LeftoverStockRepository::convertRowToReusableEntry(const QVector<QString>& parts, CsvReader::RowContext& ctx) {
    const auto rowOpt = convertRowToReusableRow(parts, ctx);
    if (!rowOpt.has_value()) return std::nullopt;

    return buildReusableEntryFromRow(rowOpt.value(), ctx);
}

bool LeftoverStockRepository::saveToCSV(const LeftoverStockRegistry& registry, const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        qWarning() << "❌ Nem sikerült megnyitni a leftover stock fájlt írásra:" << filePath;

        return false;
    }

    QTextStream out(&file);
    // Qt6 alatt automatikusan UTF-8

    out << "materialBarCode;availableLength_mm;source;optimizationId;barcode;storageBarcode\n";

    for (const auto& entry : registry.readAll()) {
        if (entry.availableLength_mm <= 0 || entry.barcode.trimmed().isEmpty())
            continue;

        // Forrás és vonalkód már getterből
        QString sourceStr = LeftoverSourceUtils::toString(entry.source);
        QString materialCode = entry.materialBarcode();
        QString barcode = entry.barcode;

        QString optIdStr;
        if (entry.source == Cutting::Result::LeftoverSource::Optimization && entry.optimizationId.has_value()) {
            optIdStr = QString::number(entry.optimizationId.value());
        }

        const auto* storage = StorageRegistry::instance().findById(entry.storageId);
        QString storageBarcode = storage ? storage->barcode : "";

        out << materialCode << ";"
            << entry.availableLength_mm << ";"
            << sourceStr << ";"
            << optIdStr << ";"
            << barcode << ";"
            << storageBarcode << "\n";
    }

    return true;
}




