#pragma once

#include <QString>
#include <QVector>
#include "../stockentry.h"
#include "../registries/stockregistry.h"
#include "../../common/csvimporter.h"
//#include "model/cutresult.h"
//#include "model/reusablestockentry.h"

class StockRepository {
public:
    /// 📥 Betöltés fájlból és feltöltés a regisztrációba
    static bool loadFromCSV(StockRegistry& registry);

    //static bool saveToCSV(const StockRegistry &registry, const QString &filePath);
    // új overload: közvetlen snapshot mentése (lock-mentes I/O lehetővé tétele)
    static bool saveToCSV(const QVector<StockEntry>& snapshot, const QString& filePath);
    static bool saveToSettingsPath(const StockRegistry &registry);
private:
    struct StockEntryRow {
        QString barcode;
        int quantity;
        QString storageBarcode;
        QString comment;

        // ⭐ ÚJ MEZŐK
        QDateTime createdAt;
        QDateTime lastSeenAt;
    };

    /// 🔒 Private parser, visszaad egy lista objektumot
    static QVector<StockEntry> loadFromCSV_private(CsvReader::FileContext& ctx);
    static std::optional<StockEntryRow>convertRowToStockEntryRow(const QVector<QString>& parts, CsvReader::FileContext& ctx);
    static std::optional<StockEntry> buildStockEntryFromRow(const StockEntryRow &row, CsvReader::FileContext& ctx);
    static std::optional<StockEntry> convertRowToStockEntry(const QVector<QString> &parts, CsvReader::FileContext& ctx);

};
