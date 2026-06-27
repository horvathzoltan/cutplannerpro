#pragma once

#include <QString>
#include <QVector>
#include <optional>
#include "common/csvimporter.h"
#include "product/model/bom_entry.h"
#include "product/registry/product_type_registry.h"
#include "product/registry/product_subtype_registry.h"
#include "product/registry/bom_registry.h"
#include "materials/model/material_family_utils.h"

class BomRepository {
public:
    static bool loadFromCSV(BomRegistry& registry);

private:
    static std::optional<BomEntry> convertRow(const QVector<QString>& parts,
                                              CsvReader::FileContext& ctx);

    static QVector<BomEntry> loadFromCSV_private(CsvReader::FileContext& ctx);
};
