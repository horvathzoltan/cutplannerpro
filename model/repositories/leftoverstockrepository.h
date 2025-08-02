#pragma once

#include <QVector>
#include <QUuid>
#include "../leftoverstockentry.h"
#include "../registries/leftoverstockregistry.h"

class LeftoverStockRepository {
public:
    /// 📥 Betöltés fájlból és feltöltés a regisztrációba
    static bool loadFromCSV(LeftoverStockRegistry& registry);

    static bool saveToCSV(const LeftoverStockRegistry &registry);
private:
    struct ReusableStockRow {
        QString materialBarcode;
        int availableLength_mm;
        LeftoverSource source;
        std::optional<int> optimizationId;
        QString barcode;
    };

    /// 🔒 Private parser, visszaad egy lista objektumot
    static QVector<LeftoverStockEntry> loadFromCSV_private(const QString& filepath);

    static std::optional<ReusableStockRow>convertRowToReusableRow(const QVector<QString>& parts, int lineIndex);
    static std::optional<LeftoverStockEntry>buildReusableEntryFromRow(const ReusableStockRow& row, int lineIndex);
    static std::optional<LeftoverStockEntry>convertRowToReusableEntry(const QVector<QString>& parts, int lineIndex);
};
