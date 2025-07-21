#pragma once

#include <QString>
#include "../registries/materialgroupregistry.h"

class MaterialGroupRepository {
public:
    static bool loadFromCsv(MaterialGroupRegistry& registry);
private:
    static QVector<MaterialGroup> loadFromCsv_private(const QString& filepath);
};
