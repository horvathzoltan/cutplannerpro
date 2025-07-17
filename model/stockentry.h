#pragma once

#include <QString>
#include <QUuid>
//#include "common/common.h"
//#include "materialregistry.h"
//#include "materialtype.h"
#include "model/materialmaster.h"


struct StockEntry {
    QUuid materialId;                 // 🔗 Kapcsolat az anyagtörzshöz
    int stockLength_mm = 0;          // 📏 ez a hossza
    int quantity = 0;                // 📦 Elérhető darabszám

    // 🔍 Kényelmi hozzáférés (pl. név, típus lekérése a registry-ből)
    QString name() const;
    ProfileCategory category() const;
    QString barcode() const;
    MaterialType type() const;
    const MaterialMaster* master() const;
};
