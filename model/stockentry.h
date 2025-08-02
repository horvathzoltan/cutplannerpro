#pragma once

#include <QColor>
#include <QString>
#include <QUuid>
#include "model/materialmaster.h"

struct StockEntry {
    QUuid entryId = QUuid::createUuid(); // 🔑 automatikus UUID generálás

    QUuid materialId;                 // 🔗 Kapcsolat az anyagtörzshöz
    int quantity = 0;                // 📦 Elérhető darabszám

    QString materialName() const;  // 📛 Anyag neve
    QString materialBarcode() const; // 🧾 Vonalkód
    MaterialType materialType() const; // 🧬 Anyagtípus
    const MaterialMaster* master() const;

    QString materialGroupName() const;
    QColor materialGroupColor() const;
};

