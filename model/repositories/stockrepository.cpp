#include "../registries/materialregistry.h"
#include "stockrepository.h"
#include <QFile>
#include <QTextStream>
#include <QDebug>
#include <common/filehelper.h>
#include <common/filenamehelper.h>
#include <common/csvimporter.h>
#include <common/settingsmanager.h>
#include <model/registries/storageregistry.h>

bool StockRepository::loadFromCSV(StockRegistry& registry) {
    auto& helper = FileNameHelper::instance();
    if (!helper.isInited()) return false;

    QString path = helper.getStockCsvFile();
    if (path.isEmpty()) {
        qWarning("Nincs elérhető stock.csv fájl");
        return false;
    }

    QVector<StockEntry> entries = loadFromCSV_private(path);
    if (entries.isEmpty()) {
        qWarning("A stock.csv fájl üres vagy hibás sorokat tartalmaz.");
        return false;
    }

    // registry.setPersist(false);
    // registry.clearAll(); // 🔄 Korábbi készlet törlése
    // for (const auto& entry : entries)
    //     registry.registerEntry(entry);

    // registry.setPersist(true);
    registry.setData(entries); // 🔧 Itt történik a készletregisztráció
    return true;
}

QVector<StockEntry>
StockRepository::loadFromCSV_private(const QString& filepath) {
    QFile file(filepath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "❌ Nem sikerült megnyitni a fájlt:" << filepath;
        return {};
    }

    QTextStream in(&file);
    in.setEncoding(QStringConverter::Utf8);

    const auto rows = FileHelper::parseCSV(&in, ';');
    return CsvImporter::processCsvRows<StockEntry>(rows, convertRowToStockEntry);
}



std::optional<StockRepository::StockEntryRow>
StockRepository::convertRowToStockEntryRow(const QVector<QString>& parts, int lineIndex) {
    if (parts.size() < 3) {
        qWarning() << QString("⚠️ Sor %1: kevés oszlop").arg(lineIndex);
        return std::nullopt;
    }

    StockEntryRow row;
    row.barcode = parts[0].trimmed();
    const QString qtyStr = parts[1].trimmed();
    row.storageBarcode = parts[2].trimmed(); // 🆕 új mező

    bool okQty = false;
    row.quantity = qtyStr.toInt(&okQty);
    if (row.barcode.isEmpty() || !okQty || row.quantity <= 0) {
        qWarning() << QString("⚠️ Sor %1: hibás barcode vagy mennyiség").arg(lineIndex);
        return std::nullopt;
    }

    return row;
}

std::optional<StockEntry>
StockRepository::buildStockEntryFromRow(const StockEntryRow& row, int lineIndex) {
    const auto* mat = MaterialRegistry::instance().findByBarcode(row.barcode);
    if (!mat) {
        qWarning() << QString("⚠️ Sor %1: ismeretlen anyag barcode '%2'")
                          .arg(lineIndex).arg(row.barcode);
        return std::nullopt;
    }

    const auto* storage = StorageRegistry::instance().findByBarcode(row.storageBarcode);
    if (!storage) {
        qWarning() << QString("⚠️ Sor %1: ismeretlen tároló barcode '%2'")
                          .arg(lineIndex).arg(row.storageBarcode);
        return std::nullopt;
    }

    StockEntry entry;
    entry.materialId = mat->id;
    entry.quantity   = row.quantity;
    entry.storageId  = storage->id; // 🔗 Tároló UUID beállítása

    return entry;
}

std::optional<StockEntry>
StockRepository::convertRowToStockEntry(const QVector<QString>& parts, int lineIndex) {
    const auto rowOpt = convertRowToStockEntryRow(parts, lineIndex);
    if (!rowOpt.has_value()) return std::nullopt;

    return buildStockEntryFromRow(rowOpt.value(), lineIndex);
}

bool StockRepository::saveToCSV(const StockRegistry& registry, const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "❌ Nem sikerült megnyitni a stock fájlt írásra:" << filePath;
        return false;
    }

    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8);

    // 🏷️ CSV fejléc
    out << "materialBarcode;quantity;storageBarcode\n";

    for (const StockEntry& entry : registry.readAll()) {
        const auto* mat = MaterialRegistry::instance().findById(entry.materialId);
        if (!mat) {
            qWarning() << "⚠️ Hiányzó anyag mentéskor:" << entry.materialId.toString();
            continue;
        }

        QString storageBarcode;
        const auto* storage = StorageRegistry::instance().findById(entry.storageId);
        if (!storage) {
            storageBarcode = storage->barcode;
        } else {
            qWarning() << "⚠️ Hiányzó tároló mentéskor:" << entry.storageId.toString();
            storageBarcode = ""; // vagy "UNKNOWN"
        }

        out << mat->barcode << ";" << entry.quantity << ";" << storageBarcode << "\n";
    }

    file.close();
    return true;
}

