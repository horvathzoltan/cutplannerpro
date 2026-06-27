#pragma once

#include <QString>
#include <QVector>
#include <optional>
#include "common/csvimporter.h"
#include "product/model/product_type.h"
#include "product/registry/product_type_registry.h"

class ProductTypeRepository {
public:
    static bool loadFromCSV(ProductTypeRegistry& registry);

private:
    static std::optional<ProductType> convertRow(const QVector<QString>& parts,
                                                 CsvReader::FileContext& ctx);

    static QVector<ProductType> loadFromCSV_private(CsvReader::FileContext& ctx);
};
