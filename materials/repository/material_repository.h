#pragma once

#include <QString>
#include <QVector>
#include <common/csvimporter.h>
#include "materials/model/material_master.h"
#include "materials/registry/material_registry.h"
#include "common/csvimporter.h"


class MaterialRepository {
public:
    static bool loadFromCSV(MaterialRegistry& registry);
    void exportCsv(const QString &path);
private:
    // 📥 CSV betöltés → visszaadja az anyagok listáját
    static QVector<MaterialMaster> loadFromCSV_private(CsvReader::FileContext& filePath);

    struct MaterialRow {
        QString name;
        QString barcode;
        QString stockLengthStr;
        QString dim1;
        QString dim2;
        QString shapeStr;
        QString machineId;
        QString typeStr;

        QString colorStr; // 🎨 Opcionális színmező (RAL, HEX vagy üres)
        QString surfaceStr;
        QString paintingMode;

        QString cuttingMode;


        // ÚJ MEZŐK
        QString trimStr;
        QString minLeftOverStr;
        QString scrapStr;
        QString goodLeftOverMinStr;
        QString goodLeftOverMaxStr;
        QString externalCodeStr;
        QString description;
        QString familyStr;

        struct ValidatorResponse_level1{
            ValidatorResponse_level1(bool a){
                ok = a;
                //len=0.0;
            }
            bool ok = false;
            double len = 0.0;
        };

        ValidatorResponse_level1 validate_level1(CsvReader::FileContext& ctx) const;
    };

    static std::optional<MaterialMaster> convertRowToMaterial(const QVector<QString>& parts, CsvReader::FileContext& ctx);
    static std::optional<MaterialRow>convertRowToMaterialRow(const QVector<QString>& parts, CsvReader::FileContext& ctx);
    static std::optional<MaterialMaster> buildMaterialFromRow(const MaterialRow &row,
                                                              const MaterialRepository::MaterialRow::ValidatorResponse_level1& v1,
                                                              CsvReader::FileContext& ctx);

    static const int FIELD_COUNT = 20;
    //static bool validate_level1(const MaterialRow& row, CsvReader::FileContext& ctx);
};
