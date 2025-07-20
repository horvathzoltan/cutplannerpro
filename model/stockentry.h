#pragma once

#include <QColor>
#include <QString>
#include <QUuid>
#include "model/materialmaster.h"

struct StockEntry {
    QUuid materialId;                 // 🔗 Kapcsolat az anyagtörzshöz
    int quantity = 0;                // 📦 Elérhető darabszám

    QString name() const;  // 📛 Anyag neve
    QString barcode() const; // 🧾 Vonalkód
    MaterialType type() const; // 🧬 Anyagtípus
    const MaterialMaster* master() const;

    QString groupName() const;
    QColor groupColor() const;
};

