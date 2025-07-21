#pragma once

#include <QString>
#include <QVector>
#include "../stockentry.h"
#include "../registries/stockregistry.h"

class StockRepository {
public:
    /// 📥 Betöltés fájlból és feltöltés a regisztrációba
    static bool loadFromCSV(StockRegistry& registry);

private:
    /// 🔒 Private parser, visszaad egy lista objektumot
    static QVector<StockEntry> loadFromCSV_private(const QString& filepath);
};
