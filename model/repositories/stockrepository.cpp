#include "materials/registry/material_registry.h"
#include "../../common/csvhelper.h"
#include "stockrepository.h"
#include <QFile>
#include <QTextStream>
#include <QDebug>
//#include "../../common/filehelper.h"
#include "../../common/filenamehelper.h"
#include "../../common/csvimporter.h"
//#include "settings/settingsmanager.h"
#include "../registries/storageregistry.h"
#include "../../common/logger.h"

bool StockRepository::loadFromCSV(StockRegistry& registry) {
    auto& helper = FileNameHelper::instance();
    if (!helper.isInited()) {
        zWarning("❌ A FileNameHelper nincs inicializálva.");
        return false;
    }

    QString path = helper.getStockCsvFile();
    if (path.isEmpty()) {
        zWarning("❌ Nincs elérhető stock.csv fájl.");
        return false;
    }

    CsvReader::FileContext ctx(path);
    QVector<StockEntry> entries = loadFromCSV_private(ctx);

    // 🔍 Hibák loggolása
    if (ctx.hasErrors()) {
        zWarning(QString("⚠️ Hibák az importálás során (%1 sor):").arg(ctx.errorsSize()));
        zWarning(ctx.toString());
    }

    if (entries.isEmpty()) {
        zWarning("❌ A stock.csv fájl üres vagy hibás sorokat tartalmaz.");
        return false;
    }

    // registry.setPersist(false);
    // registry.clearAll(); // 🔄 Korábbi készlet törlése
    // for (const auto& entry : entries)
    //     registry.registerEntry(entry);

    // registry.setPersist(true);
    registry.setData(entries, false); // 🔧 Itt történik a készletregisztráció
    zInfo(L("✅ %1 készletbejegyzés sikeresen importálva a fájlból: %2").arg(entries.size()).arg(path));
    return true;
}

QVector<StockEntry>
StockRepository::loadFromCSV_private(CsvReader::FileContext& ctx) {
    // QFile file(filepath);
    // if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    //     qWarning() << "❌ Nem sikerült megnyitni a fájlt:" << filepath;
    //     return {};
    // }

    // QTextStream in(&file);
    // in.setEncoding(QStringConverter::Utf8);

    // const auto rows = FileHelper::parseCSV(&in, ';');
    // return CsvImporter::processCsvRows<StockEntry>(rows, convertRowToStockEntry);
    return CsvReader::readAndConvert<StockEntry>(ctx, convertRowToStockEntry, true);
}



std::optional<StockRepository::StockEntryRow>
StockRepository::convertRowToStockEntryRow(const QVector<QString>& parts, CsvReader::FileContext& ctx) {
    if (parts.size() < 6) {
        QString msg = L("⚠️ Kevés oszlop");
        ctx.addError(ctx.currentLineNumber(), msg);
        return std::nullopt;
    }

    if (parts.size() > 6) {
        QString msg = L("⚠️ Tűl sok oszlop");
        ctx.addError(ctx.currentLineNumber(), msg);
        return std::nullopt;
    }

    StockEntryRow row;
    row.barcode = parts[0].trimmed();
    const QString qtyStr = parts[1].trimmed();
    row.storageBarcode = parts[2].trimmed(); // 🆕 új mező

    // 💬 opcionális 4. oszlop: comment
    row.comment = (parts.size() >= 4) ? parts[3].trimmed() : QString();

    bool okQty = false;
    row.quantity = qtyStr.toInt(&okQty);
    if (row.barcode.isEmpty() || !okQty || row.quantity <= 0) {
        QString msg = L("⚠️ Sor %1: hibás barcode vagy mennyiség");
        ctx.addError(ctx.currentLineNumber(), msg);
        return std::nullopt;
    }

    // ⭐ ÚJ: createdAt
    if (parts.size() >= 5) {
        row.createdAt = QDateTime::fromString(parts[4].trimmed(), "yyyy-MM-dd HH:mm:ss");
        if (!row.createdAt.isValid())
            row.createdAt = QDateTime::currentDateTime();
    } else {
        row.createdAt = QDateTime::currentDateTime();
    }

    // ⭐ ÚJ: lastSeenAt
    if (parts.size() >= 6) {
        row.lastSeenAt = QDateTime::fromString(parts[5].trimmed(), "yyyy-MM-dd HH:mm:ss");
        if (!row.lastSeenAt.isValid())
            row.lastSeenAt = row.createdAt;
    } else {
        row.lastSeenAt = row.createdAt;
    }
    return row;
}

std::optional<StockEntry>
StockRepository::buildStockEntryFromRow(const StockEntryRow& row, CsvReader::FileContext& ctx) {
    const auto* mat = MaterialRegistry::instance().findByBarcode(row.barcode);
    if (!mat) {
        QString msg = L("⚠️ Ismeretlen anyag barcode '%1'").arg(row.barcode);
        ctx.addError(ctx.currentLineNumber(), msg);

        return std::nullopt;
    }

    const auto* storage = StorageRegistry::instance().findByBarcode(row.storageBarcode);
    if (!storage) {
        QString msg = L("⚠️ Ismeretlen tároló barcode '%1'").arg(row.storageBarcode);
        ctx.addError(ctx.currentLineNumber(), msg);

        return std::nullopt;
    }

    StockEntry entry;
    entry.materialId = mat->id;
    entry.quantity   = row.quantity;
    entry.storageId  = storage->id; // 🔗 Tároló UUID beállítása
    entry.comment = row.comment; // 🆕 új mező

    // ⭐ ÚJ: dátumok átadása
    entry.createdAt  = row.createdAt;
    entry.lastSeenAt = row.lastSeenAt;

    return entry;
}

std::optional<StockEntry>
StockRepository::convertRowToStockEntry(const QVector<QString>& parts, CsvReader::FileContext& ctx) {
    const auto rowOpt = convertRowToStockEntryRow(parts, ctx);
    if (!rowOpt.has_value()) return std::nullopt;

    return buildStockEntryFromRow(rowOpt.value(), ctx);
}

// bool StockRepository::saveToCSV(const StockRegistry& registry, const QString& filePath) {
//     QFile file(filePath);
//     if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
//         qWarning() << "❌ Nem sikerült megnyitni a stock fájlt írásra:" << filePath;
//         return false;
//     }

//     QTextStream out(&file);
//     out.setEncoding(QStringConverter::Utf8);

//     out << "materialBarcode;quantity;storageBarcode;comment\n";

//     const auto entries = registry.readAll(); // ha lehet, érdemes const&-re váltani a readAll() visszatérési típusát
//     for (const StockEntry& entry : entries) {
//         const auto* mat = MaterialRegistry::instance().findById(entry.materialId);
//         if (!mat) {
//             qWarning() << "⚠️ Hiányzó anyag mentéskor:" << entry.materialId.toString();
//             continue;
//         }

//         const auto* storage = StorageRegistry::instance().findById(entry.storageId);
//         const QString storageBarcode = storage ? storage->barcode : QString();

//         out << mat->barcode << ';'
//             << entry.quantity << ';'
//             << storageBarcode << ';'
//             << CsvHelper::escape(entry.comment) << '\n';
//     }

//     return true;
// }

bool StockRepository::saveToCSV(const QVector<StockEntry>& snapshot, const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "❌ Nem sikerült megnyitni a stock fájlt írásra:" << filePath;
        return false;
    }

    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8);

    //out << "materialBarcode;quantity;storageBarcode;comment\n";
    out << "materialBarcode;quantity;storageBarcode;comment;createdAt;lastSeenAt\n";


    for (const StockEntry& entry : snapshot) {
        const auto* mat = MaterialRegistry::instance().findById(entry.materialId);
        if (!mat) {
            qWarning() << "⚠️ Hiányzó anyag mentéskor:" << entry.materialId.toString();
            continue;
        }

        const auto* storage = StorageRegistry::instance().findById(entry.storageId);
        const QString storageBarcode = storage ? storage->barcode : QString();

        out << mat->barcode << ';'
            << entry.quantity << ';'
            << storageBarcode << ';'
            << CsvHelper::escape(entry.comment) << ';'
            << entry.createdAt.toString("yyyy-MM-dd HH:mm:ss") << ';'
            << entry.lastSeenAt.toString("yyyy-MM-dd HH:mm:ss")
            << '\n';

    }

    return true;
}


