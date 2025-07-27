#include "reusablestockrepository.h"
#include <QFile>
#include <QTextStream>
#include <QDebug>
#include "common/filenamehelper.h"
#include <model/registries/materialregistry.h>
#include <common/filehelper.h>
#include <common/csvimporter.h>

bool ReusableStockRepository::loadFromCSV(ReusableStockRegistry& registry) {
    auto& helper = FileNameHelper::instance();
    if (!helper.isInited()) return false;

    QString path = helper.getLeftoversCsvFile();
    if (path.isEmpty()) {
        qWarning("Nincs elérhető leftovers.csv fájl");
        return false;
    }

    QVector<ReusableStockEntry> entries = loadFromCSV_private(path);
    if (entries.isEmpty()) {
        qWarning("A leftovers.csv fájl üres vagy hibás sorokat tartalmaz.");
        return false;
    }

    registry.clear(); // 🔄 Korábbi készlet törlése
    for (const auto& entry : entries)
        registry.add(entry);

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

QVector<ReusableStockEntry>
ReusableStockRepository::loadFromCSV_private(const QString& filepath) {
    QFile file(filepath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "❌ Nem sikerült megnyitni a leftovers fájlt:" << filepath;
        return {};
    }

    QTextStream in(&file);
    in.setEncoding(QStringConverter::Utf8);
    const auto rows = FileHelper::parseCSV(&in, ';');

    return CsvImporter::processCsvRows<ReusableStockEntry>(rows, convertRowToReusableEntry);
}


std::optional<ReusableStockRepository::ReusableStockRow>
ReusableStockRepository::convertRowToReusableRow(const QVector<QString>& parts, int lineIndex) {
    if (parts.size() < 5) {
        qWarning() << QString("⚠️ Sor %1: kevés oszlop").arg(lineIndex);
        return std::nullopt;
    }

    ReusableStockRow row;
    row.materialBarcode = parts[0].trimmed();

    bool okLength = false;
    row.availableLength_mm = parts[1].trimmed().toInt(&okLength);
    if (row.materialBarcode.isEmpty() || !okLength || row.availableLength_mm <= 0) {
        qWarning() << QString("⚠️ Sor %1: hibás barcode vagy hossz").arg(lineIndex);
        return std::nullopt;
    }

    const QString sourceStr = parts[2].trimmed();
    if (sourceStr == "Optimization")
        row.source = LeftoverSource::Optimization;
    else if (sourceStr == "Manual")
        row.source = LeftoverSource::Manual;
    else {
        qWarning() << QString("⚠️ Sor %1: ismeretlen forrástípus").arg(lineIndex);
        return std::nullopt;
    }

    if (row.source == LeftoverSource::Optimization && parts.size() > 3) {
        bool okOpt = false;
        const int optId = parts[3].trimmed().toInt(&okOpt);
        if (okOpt)
            row.optimizationId = optId;
        else {
            qWarning() << QString("⚠️ Sor %1: hibás optimalizáció ID").arg(lineIndex);
            return std::nullopt;
        }
    }

    row.barcode = parts[4].trimmed();
    if (row.barcode.isEmpty()) {
        qWarning() << QString("⚠️ Sor %1: hiányzó egyedi barcode").arg(lineIndex);
        return std::nullopt;
    }

    return row;
}

std::optional<ReusableStockEntry>
ReusableStockRepository::buildReusableEntryFromRow(const ReusableStockRow& row, int lineIndex) {
    const auto* mat = MaterialRegistry::instance().findByBarcode(row.materialBarcode);
    if (!mat) {
        qWarning() << QString("⚠️ Sor %1: ismeretlen anyag barcode '%2'").arg(lineIndex).arg(row.materialBarcode);
        return std::nullopt;
    }

    ReusableStockEntry entry;
    entry.materialId         = mat->id;
    entry.availableLength_mm = row.availableLength_mm;
    entry.barcode            = row.barcode;
    entry.source             = row.source;
    entry.optimizationId     = row.optimizationId;

    return entry;
}

std::optional<ReusableStockEntry>
ReusableStockRepository::convertRowToReusableEntry(const QVector<QString>& parts, int lineIndex) {
    const auto rowOpt = convertRowToReusableRow(parts, lineIndex);
    if (!rowOpt.has_value()) return std::nullopt;

    return buildReusableEntryFromRow(rowOpt.value(), lineIndex);
}



// std::optional<ReusableStockEntry> ReusableStockRepository::convertRowToReusableEntry(const QVector<QString>& parts, int lineIndex) {
//     if (parts.size() < 5) {
//         qWarning() << QString("⚠️ Sor %1 hibás (kevés oszlop):").arg(lineIndex) << parts;
//         return std::nullopt;
//     }

//     const QString materialBarCode = parts[0].trimmed();
//     const auto* mat = MaterialRegistry::instance().findByBarcode(materialBarCode);
//     if (!mat) {
//         qWarning() << QString("⚠️ Ismeretlen anyag sor %1-ben:").arg(lineIndex) << materialBarCode;
//         return std::nullopt;
//     }

//     bool okQty = false;
//     const int availableLength_mm = parts[1].trimmed().toInt(&okQty);
//     if (!okQty || availableLength_mm <= 0) {
//         qWarning() << QString("⚠️ Sor %1, oszlop 2: érvénytelen hosszérték").arg(lineIndex);
//         return std::nullopt;
//     }

//     const QString sourceStr = parts[2].trimmed();
//     LeftoverSource source;
//     if (sourceStr == "Optimization") {
//         source = LeftoverSource::Optimization;
//     } else if (sourceStr == "Manual") {
//         source = LeftoverSource::Manual;
//     } else {
//         qWarning() << QString("⚠️ Sor %1, oszlop 3: ismeretlen forrástípus").arg(lineIndex);
//         return std::nullopt;
//     }

//     std::optional<int> optimizationId = std::nullopt;
//     if (source == LeftoverSource::Optimization && parts.size() > 3) {
//         bool okOptId = false;
//         const int optId = parts[3].trimmed().toInt(&okOptId);
//         if (okOptId)
//             optimizationId = optId;
//         else {
//             qWarning() << QString("⚠️ Sor %1, oszlop 4: hibás optimalizáció ID").arg(lineIndex);
//             return std::nullopt;
//         }
//     }

//     const QString barcode = parts[4].trimmed();
//     if (barcode.isEmpty()) {
//         qWarning() << QString("⚠️ Sor %1, oszlop 5: hiányzó egyedi barcode").arg(lineIndex);
//         return std::nullopt;
//     }

//     ReusableStockEntry entry;
//     entry.materialId = mat->id;
//     entry.availableLength_mm = availableLength_mm;
//     entry.barcode = barcode;
//     entry.source = source;
//     entry.optimizationId = optimizationId;
//     return entry;
// }


// QVector<ReusableStockEntry> ReusableStockRepository::loadFromCSV_private(const QString& filepath) {
//     QVector<ReusableStockEntry> result;
//     QFile file(filepath);

//     if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
//         qWarning() << "❌ Nem sikerült megnyitni a leftovers fájlt:" << filepath;
//         return result;
//     }

//     QTextStream in(&file);
//     bool firstLine = true;

//     while (!in.atEnd()) {
//         QString line = in.readLine().trimmed();
//         if (line.isEmpty()) continue;
//         if (firstLine) { firstLine = false; continue; }

//         const QStringList cols = line.split(';');
//         if (cols.size() < 5) continue;

//         QString materialBarCode = cols[0].trimmed();
//         bool okQty = false;
//         int availableLength_mm = cols[1].toInt(&okQty);
//         if (!okQty || availableLength_mm <= 0) continue;

//         QString sourceStr = cols[2].trimmed();
//         LeftoverSource source = (sourceStr == "Optimization") ? LeftoverSource::Optimization : LeftoverSource::Manual;

//         std::optional<int> optimizationId = std::nullopt;
//         if (source == LeftoverSource::Optimization && cols.size() > 3) {
//             bool okOptId = false;
//             int optId = cols[3].toInt(&okOptId);
//             if (okOptId) optimizationId = optId;
//         }

//         QString barcode = cols[4].trimmed();

//         const MaterialMaster* mat = MaterialRegistry::instance().findByBarcode(materialBarCode);
//         if (!mat) {
//             qWarning() << "⚠️ Hiányzó anyag barcode alapján:" << materialBarCode;
//             continue;
//         }

//         ReusableStockEntry entry;
//         entry.materialId = mat->id;
//         entry.availableLength_mm = availableLength_mm;
//         entry.barcode = barcode;
//         entry.source = source;
//         entry.optimizationId = optimizationId;
//         result.append(entry);
//     }

//     file.close();
//     return result;
// }
