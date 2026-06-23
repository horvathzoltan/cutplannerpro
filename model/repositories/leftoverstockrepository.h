#pragma once

#include <QVector>
#include <QUuid>
#include "../leftoverstockentry.h"
#include "../registries/leftoverstockregistry.h"
#include "../../common/csvimporter.h"

class LeftoverStockRepository {
public:
    /// 📥 Betöltés fájlból és feltöltés a regisztrációba
    static bool loadFromCSV(LeftoverStockRegistry& registry);

    static bool saveToCSV(const LeftoverStockRegistry &registry, const QString& filePath);
private:
    struct ReusableStockRow {
        QString materialBarcode;
        int availableLength_mm;
        Cutting::Result::LeftoverSource source;
        std::optional<int> optimizationId;
        QString barcode;
        QString storageBarcode; // 🆕 új mező

        QString createdAtStr;
        QString lastSeenAtStr;
        QString statusStr;
    };

    /// 🔒 Private parser, visszaad egy lista objektumot
    static QVector<LeftoverStockEntry> loadFromCSV_private(CsvReader::FileContext& ctx);

    static std::optional<ReusableStockRow>convertRowToReusableRow(const QVector<QString>& parts, CsvReader::FileContext& ctx);
    static std::optional<LeftoverStockEntry>buildReusableEntryFromRow(const ReusableStockRow& row, CsvReader::FileContext& ctx);
    static std::optional<LeftoverStockEntry>convertRowToReusableEntry(const QVector<QString>& parts, CsvReader::FileContext& ctx);
};
