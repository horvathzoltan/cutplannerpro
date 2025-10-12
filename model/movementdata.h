#pragma once

#include <QUuid>
#include <QString>

#include <model/registries/stockregistry.h>

struct MovementData_Log {
    // Honnan mozgatjuk
    QString fromStorageName;     // raktárhely neve
    QString fromBarcode;         // raktárhely vonalkódja (opcionális, ha van)

    // Hová mozgatjuk
    QString toStorageName;
    QString toBarcode;

    // Mit mozgatunk
    QString itemName;            // termék neve
    QString itemBarcode;         // termék vonalkódja
};


/*
Mozgatásfajták (esetek rövid listája)
Move (entry → entry): fromEntryId + toEntryId megvannak — cél entry aggregálódik.

Transfer to storage (entry → storage): fromEntryId + toStorageId — először ensureTargetEntry, majd move entry→entry.

Consume / withdraw (entry → nulla): csak fromEntryId — mennyiség levonása vagy entry törlése.

Deposit / add-only (külső anyag → storage): toStorageId + materialId (fromEntryId nincs) — ensureTargetEntry majd aggregálás.

Hiba/invalid: sem forrás, sem cél nincs, vagy quantity<=0.
*/
struct MovementData {
    // Honnan mozgatjuk
    QUuid fromEntryId;           // forrás StockEntry azonosítója (nullable)

    // Hová mozgatjuk
    QUuid toEntryId;             // konkrét cél StockEntry azonosítója (nullable)
    QUuid toStorageId;           // cél Storage azonosítója, ha még nincs cél entry (nullable)

    // Mit mozgatunk
    QUuid materialId;            // explicit materialId, használatos deposit/import esetén (nullable)
    // Mennyit
    int quantity = 0;

    // Egyéb
    QString comment;
};
