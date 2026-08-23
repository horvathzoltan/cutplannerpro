#pragma once

#include <QString>
#include <QVector>
#include <optional>

#include "materialbundles/model/bundle_definition.h"
#include "materialbundles/registry/bundle_registry.h"
#include "common/csvimporter.h"
#include "materials/registry/material_registry.h"

class BundleRepository {
public:
    /// Fő belépési pont: betölti a bundle metaadatokat és komponenseket CSV-ből
    static bool loadFromCsv(BundleRegistry& registry);

private:
    // --- Meta CSV sor ---
    struct BundleRow {
        QString bundleCode;
        QString bundleName;
    };

    // --- Komponens CSV sor ---
    struct BundleComponentRow {
        QString bundleCode;
        QString materialBarcode;
        int     count = 1;
        double  baseLength_mm = 0.0;
    };

    // --- Stage 1: Convert ---
    static std::optional<BundleRow>
    convertRowToBundleRow(const QVector<QString>& parts, CsvReader::FileContext& ctx);

    static std::optional<BundleComponentRow>
    convertRowToBundleComponentRow(const QVector<QString>& parts, CsvReader::FileContext& ctx);

    // --- Stage 2: Build ---
    static std::optional<BundleDefinition>
    buildBundleFromRow(const BundleRow& row, CsvReader::FileContext& ctx);

    static std::optional<BundleComponent>
    buildComponentFromRow(const BundleComponentRow& row, CsvReader::FileContext& ctx);

    // --- Stage 3: Load & Assemble ---
    static QVector<BundleRow> loadBundleRows(CsvReader::FileContext& ctx);
    static QVector<BundleComponentRow> loadComponentRows(CsvReader::FileContext& ctx);
};
