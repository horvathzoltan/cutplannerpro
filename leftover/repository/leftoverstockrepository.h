#pragma once

#include <QVector>
#include <QUuid>
#include "leftover/model/leftoverstockentry.h"
#include "leftover/registry/leftoverstockregistry.h"
#include "../../common/csvimporter.h"

class LeftoverStockRepository {
public:
    /// 📥 Betöltés fájlból és feltöltés a regisztrációba
    static bool loadFromCSV(LeftoverStockRegistry& registry);

    static bool saveToCSV(const LeftoverStockRegistry &registry, const QString& filePath);
private:
    struct LeftoverStockEntry_Row {
        QString materialBarcode;
        int availableLength_mm;
        Cutting::Result::LeftoverSource source;
        std::optional<int> optimizationId;
        QString barcode;
        QString storageBarcode; // 🆕 új mező

        QString createdAtStr;
        QString lastSeenAtStr;
        QString statusStr;
        int notFoundCount = 0;

        QString bundleComponentLengthsCsv;   // ⭐ ÚJ
    };

    /// 🔒 Private parser, visszaad egy lista objektumot
    static QVector<LeftoverStockEntry> loadFromCSV_private(CsvReader::FileContext& ctx);

    static std::optional<LeftoverStockEntry_Row>convertRowToReusableRow(const QVector<QString>& parts, CsvReader::FileContext& ctx);
    static std::optional<LeftoverStockEntry>buildReusableEntryFromRow(const LeftoverStockEntry_Row& row, CsvReader::FileContext& ctx);
    static std::optional<LeftoverStockEntry>convertRowToReusableEntry(const QVector<QString>& parts, CsvReader::FileContext& ctx);
};
