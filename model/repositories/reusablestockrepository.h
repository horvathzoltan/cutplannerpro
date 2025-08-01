#pragma once

#include <QVector>
#include <QUuid>
#include "../reusablestockentry.h"
#include "../registries/reusablestockregistry.h"

class ReusableStockRepository {
public:
    /// 📥 Betöltés fájlból és feltöltés a regisztrációba
    static bool loadFromCSV(ReusableStockRegistry& registry);

    static bool saveToCSV(const ReusableStockRegistry &registry);
private:
    struct ReusableStockRow {
        QString materialBarcode;
        int availableLength_mm;
        LeftoverSource source;
        std::optional<int> optimizationId;
        QString barcode;
    };

    /// 🔒 Private parser, visszaad egy lista objektumot
    static QVector<ReusableStockEntry> loadFromCSV_private(const QString& filepath);

    static std::optional<ReusableStockRow>convertRowToReusableRow(const QVector<QString>& parts, int lineIndex);
    static std::optional<ReusableStockEntry>buildReusableEntryFromRow(const ReusableStockRow& row, int lineIndex);
    static std::optional<ReusableStockEntry>convertRowToReusableEntry(const QVector<QString>& parts, int lineIndex);
};
