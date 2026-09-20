#pragma once

#include <QString>
#include <QList>
#include <optional>

#include "product/model/product_calcmodes.h"
#include "product/registry/product_calcmode_registry.h"
#include "common/csvimporter.h"

class ProductCalcModeRepository {
public:
    static bool loadFromMsff(ProductCalcModeRegistry& registry);

private:
    struct ParentRow {
        QString typeCode;
        QString subtypeCode;
    };

    struct ChildRow {
        QString modeString;   // pl. "Gyartasi" vagy "Gyartasi;*"
    };

    // --- Stage 1: Convert ---
    static std::optional<ParentRow>
    convertParentRow(const QVector<QString>& parts, CsvReader::FileContext& ctx);

    static std::optional<ChildRow>
    convertChildRow(const QVector<QString>& parts, CsvReader::FileContext& ctx);

    // --- Stage 2: Build ---
    static std::optional<ProductCalcModes>
    buildEntry(const ParentRow& parent,
               const QList<ChildRow>& children,
               CsvReader::FileContext& ctx);

    // --- Stage 3: Load & Assemble ---
    static QList<QList<QVector<QString>>>
    readMsffSections(const QString& filepath);

    static bool parseMsffSections(const QList<QList<QVector<QString>>>& sections,
                                  ProductCalcModeRegistry& registry);
};
