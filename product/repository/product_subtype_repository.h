#pragma once

#include <QString>
#include <QVector>
#include <optional>
#include "common/csvimporter.h"
#include "product/model/product_subtype.h"
#include "product/registry/product_type_registry.h"
#include "product/registry/product_subtype_registry.h"

class ProductSubtypeRepository {
public:
    static bool loadFromCSV(ProductSubtypeRegistry& registry);

private:
    static std::optional<ProductSubtype> convertRow(const QVector<QString>& parts,
                                                    CsvReader::FileContext& ctx);

    static QVector<ProductSubtype> loadFromCSV_private(CsvReader::FileContext& ctx);
};
