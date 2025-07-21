#pragma once

#include <QVector>
#include <QUuid>
#include "../reusablestockentry.h"
#include "../registries/reusablestockregistry.h"

class ReusableStockRepository {
public:
    /// 📥 Betöltés fájlból és feltöltés a regisztrációba
    static bool loadFromCSV(ReusableStockRegistry& registry);

private:
    /// 🔒 Private parser, visszaad egy lista objektumot
    static QVector<ReusableStockEntry> loadFromCSV_private(const QString& filepath);
};
