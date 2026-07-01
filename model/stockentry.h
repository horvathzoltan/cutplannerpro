#pragma once

#include <QColor>
#include <QDateTime>
#include <QString>
#include <QUuid>
#include "materials/model/material_master.h"
#include "storageentry.h"

struct StockEntry {
    QUuid entryId = QUuid::createUuid(); // 🔑 automatikus UUID generálás

    QUuid materialId;                 // 🔗 Kapcsolat az anyagtörzshöz
    int quantity = 0;                // 📦 Elérhető darabszám
    QUuid storageId;                // 📦 Tárolási hely azonosítója

    QString comment; // 💬 Felhasználói megjegyzés

    // ⭐ ÚJ MEZŐK
    QDateTime createdAt = QDateTime::currentDateTime();
    QDateTime lastSeenAt = QDateTime::currentDateTime();

    QString materialName() const;  // 📛 Anyag neve
    QString materialBarcode() const; // 🧾 Vonalkód
    MaterialType materialType() const; // 🧬 Anyagtípus
    const MaterialMaster* master() const;

    QString materialGroupName() const;
    QColor materialGroupColor() const;

    QString storageName() const;  // 📛 Anyag neve
    QString storageBarcode() const; // 🧾 Vonalkód
    const StorageEntry *storage() const;
};

