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
private:
    // 📥 CSV betöltés → visszaadja az anyagok listáját
    static QVector<MaterialMaster> loadFromCSV_private(CsvReader::FileContext& filePath);

    struct MaterialRow {
        QString name;
        QString barcode;
        double stockLength;
        QString dim1;
        QString dim2;
        QString shapeStr;
        QString machineId;
        QString typeStr;
        QString colorStr; // 🎨 Opcionális színmező (RAL, HEX vagy üres)
        QString cuttingMode;
        QString paintingMode;

        // ÚJ MEZŐK
        QString trimStr;
        QString minLeftOverStr;
        QString scrapStr;
        QString goodLeftOverMinStr;
        QString goodLeftOverMaxStr;
        QString externalCodeStr;
    };

    static std::optional<MaterialMaster> convertRowToMaterial(const QVector<QString>& parts, CsvReader::FileContext& ctx);
    static std::optional<MaterialRow>convertRowToMaterialRow(const QVector<QString>& parts, CsvReader::FileContext& ctx);
    static std::optional<MaterialMaster> buildMaterialFromRow(const MaterialRow &row, CsvReader::FileContext& ctx);
};
