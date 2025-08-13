#pragma once

#include <QUuid>
#include <QString>

struct MovementData {
    // Honnan mozgatjuk
    QUuid fromEntryId;           // belső azonosító
    QString fromStorageName;     // raktárhely neve
    QString fromBarcode;         // raktárhely vonalkódja (opcionális, ha van)

    // Hová mozgatjuk
    QUuid toStorageId;
    QString toStorageName;
    QString toBarcode;

    // Mit mozgatunk
    QString itemName;            // termék neve
    QString itemBarcode;         // termék vonalkódja

    // Mennyit
    int quantity = 0;

    // Egyéb
    QString comment;
};
