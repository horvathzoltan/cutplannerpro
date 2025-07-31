#include "../registries/materialregistry.h"
#include "stockrepository.h"
#include <QFile>
#include <QTextStream>
#include <QDebug>
#include <common/filehelper.h>
#include <common/filenamehelper.h>
#include <common/csvimporter.h>
#include <common/settingsmanager.h>

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

    registry.clear(); // 🔄 Korábbi készlet törlése
    for (const auto& entry : entries)
        registry.add(entry);

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
    if (parts.size() < 2) {
        qWarning() << QString("⚠️ Sor %1: kevés oszlop").arg(lineIndex);
        return std::nullopt;
    }

    StockEntryRow row;
    row.barcode = parts[0].trimmed();
    const QString qtyStr = parts[1].trimmed();

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

    StockEntry entry;
    entry.materialId = mat->id;
    entry.quantity   = row.quantity;
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
    out << "materialBarcode;quantity\n";

    for (const StockEntry& entry : registry.all()) {
        const auto* mat = MaterialRegistry::instance().findById(entry.materialId);
        if (!mat) {
            qWarning() << "⚠️ Hiányzó anyag mentéskor:" << entry.materialId.toString();
            continue;
        }

        out << mat->barcode << ";" << entry.quantity << "\n";
    }

    file.close();
    return true;
}

