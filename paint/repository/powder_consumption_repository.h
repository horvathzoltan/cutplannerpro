#pragma once

#include <QString>
#include <QVector>
#include <optional>

#include "paint/model/powder_consumption_model.h"
#include "paint/registry/powder_consumption_registry.h"
#include "common/csvimporter.h"
#include "common/logger.h"
#include <product/registry/product_type_registry.h>
#include <product/registry/product_subtype_registry.h>

class PowderConsumptionRepository
{
public:
    static bool loadFromCSV(PowderConsumptionRegistry& registry);

private:
    struct Row {
        QString typeCode;
        QString subtypeCode;
        QString lengthStr;
        QString weightStr;
        QString colorMultStr;
        QString surfaceMultStr;
    };

    static std::optional<Row> parseRow(const QVector<QString>& parts,
                                       CsvReader::FileContext& ctx);

    static std::optional<PowderConsumptionModel> buildModel(const Row& row,
                                                            CsvReader::FileContext& ctx);

    static QVector<PowderConsumptionModel> loadFromCSV_private(CsvReader::FileContext& ctx);
};
