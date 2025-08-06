#pragma once

#include <QString>
#include <QVector>
#include "../stockentry.h"
#include "../registries/stockregistry.h"
//#include "model/cutresult.h"
//#include "model/reusablestockentry.h"

class StockRepository {
public:
    /// 📥 Betöltés fájlból és feltöltés a regisztrációba
    static bool loadFromCSV(StockRegistry& registry);

    static bool saveToCSV(const StockRegistry &registry, const QString &filePath);
    static bool saveToSettingsPath(const StockRegistry &registry);
private:
    struct StockEntryRow {
        QString barcode;
        int quantity;
        QString storageBarcode;
    };

    /// 🔒 Private parser, visszaad egy lista objektumot
    static QVector<StockEntry> loadFromCSV_private(const QString& filepath);    
    static std::optional<StockEntryRow>convertRowToStockEntryRow(const QVector<QString>& parts, int lineIndex);
    static std::optional<StockEntry> buildStockEntryFromRow(const StockEntryRow &row, int lineIndex);
    static std::optional<StockEntry> convertRowToStockEntry(const QVector<QString> &parts, int lineIndex);
};
