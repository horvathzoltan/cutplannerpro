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
};
