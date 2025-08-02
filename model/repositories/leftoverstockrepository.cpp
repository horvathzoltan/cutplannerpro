#include "leftoverstockrepository.h"
#include <QFile>
#include <QTextStream>
#include <QDebug>
#include "common/filenamehelper.h"
#include "common/leftoversourceutils.h"
#include <model/registries/materialregistry.h>
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

QVector<LeftoverStockEntry>
LeftoverStockRepository::loadFromCSV_private(const QString& filepath) {
    QFile file(filepath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "❌ Nem sikerült megnyitni a leftovers fájlt:" << filepath;
        return {};
    }

    QTextStream in(&file);
    in.setEncoding(QStringConverter::Utf8);
    const auto rows = FileHelper::parseCSV(&in, ';');

    return CsvImporter::processCsvRows<LeftoverStockEntry>(rows, convertRowToReusableEntry);
}


std::optional<LeftoverStockRepository::ReusableStockRow>
LeftoverStockRepository::convertRowToReusableRow(const QVector<QString>& parts, int lineIndex) {
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
    if (row.source == LeftoverSource::Undefined) {
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

std::optional<LeftoverStockEntry>
LeftoverStockRepository::buildReusableEntryFromRow(const ReusableStockRow& row, int lineIndex) {
    const auto* mat = MaterialRegistry::instance().findByBarcode(row.materialBarcode);
    if (!mat) {
        qWarning() << QString("⚠️ Sor %1: ismeretlen anyag barcode '%2'").arg(lineIndex).arg(row.materialBarcode);
        return std::nullopt;
    }

    LeftoverStockEntry entry;
    entry.materialId         = mat->id;
    entry.availableLength_mm = row.availableLength_mm;
    entry.barcode            = row.barcode;
    entry.source             = row.source;
    entry.optimizationId     = row.optimizationId;

    return entry;
}

std::optional<LeftoverStockEntry>
LeftoverStockRepository::convertRowToReusableEntry(const QVector<QString>& parts, int lineIndex) {
    const auto rowOpt = convertRowToReusableRow(parts, lineIndex);
    if (!rowOpt.has_value()) return std::nullopt;

    return buildReusableEntryFromRow(rowOpt.value(), lineIndex);
}

bool LeftoverStockRepository::saveToCSV(const LeftoverStockRegistry& registry) {
    QFile file("leftovers.csv");
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return false;

    QTextStream out(&file);
    // Qt6 alatt automatikusan UTF-8

    out << "materialBarCode;availableLength_mm;source;optimizationId;barcode\n";

    for (const auto& entry : registry.all()) {
        if (entry.availableLength_mm <= 0 || entry.barcode.trimmed().isEmpty())
            continue;

        // Forrás és vonalkód már getterből
        QString sourceStr = LeftoverSourceUtils::toString(entry.source);
        QString materialCode = entry.materialBarcode();
        QString barcode = entry.barcode;

        QString optIdStr;
        if (entry.source == LeftoverSource::Optimization && entry.optimizationId.has_value()) {
            optIdStr = QString::number(entry.optimizationId.value());
        }

        out << materialCode << ";"
            << entry.availableLength_mm << ";"
            << sourceStr << ";"
            << optIdStr << ";"
            << barcode << "\n";
    }

    return true;
}




