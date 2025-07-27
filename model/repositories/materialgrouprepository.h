#pragma once

#include <QString>
#include "../registries/materialgroupregistry.h"

class MaterialGroupRepository {
public:
    static bool loadFromCsv(MaterialGroupRegistry& registry);
private:
    struct MaterialGroupRow {
        QString logicalId; // ez mondja meg hogy ez milyen csoport  - ez a csoport neve
        QString displayName;
        QString barcode;
        QString colorHex;
    };

    static QVector<MaterialGroup> loadFromCsv_private(const QString& filepath);

    static std::optional<MaterialGroupRow> convertRowToMaterialGroupRow(const QVector<QString>& parts, int lineIndex);
    static std::optional<MaterialGroup> buildMaterialGroupFromRow(const MaterialGroupRow& row, int lineIndex);
};
