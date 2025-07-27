#pragma once

#include <QString>
#include <QVector>
#include "../materialmaster.h"
#include "../registries/materialregistry.h"

class MaterialRepository {
public:
    static bool loadFromCSV(MaterialRegistry& registry);
private:
    // 📥 CSV betöltés → visszaadja az anyagok listáját
    static QVector<MaterialMaster> loadFromCSV_private(const QString& filePath);

    struct MaterialRow {
        QString name;
        QString barcode;
        double stockLength;
        QString dim1;
        QString dim2;
        QString shapeStr;
        QString machineId;
        QString typeStr;
    };

    static std::optional<MaterialMaster> convertRowToMaterial(const QVector<QString>& parts, int lineIndex);
    static std::optional<MaterialRow>convertRowToMaterialRow(const QVector<QString>& parts, int lineIndex);
    static std::optional<MaterialMaster> buildMaterialFromRow(const MaterialRow &row, int lineIndex);
};
