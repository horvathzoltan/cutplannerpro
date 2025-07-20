#pragma once

#include <QString>
#include "materialgroupregistry.h"

class MaterialGroupRepository {
public:
    static bool loadFromCsv(MaterialGroupRegistry& registry);
private:
    static QVector<MaterialGroup> loadFromCsv_private(const QString& filepath);
};
