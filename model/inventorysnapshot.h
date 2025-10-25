#pragma once

#include <QVector>
#include "stockentry.h"
#include "leftoverstockentry.h"

/**
 * @brief A készlet pillanatképe (snapshot), amin az optimalizáció dolgozik.
 *
 * Ez egy egyszerű adatstruktúra, amely tartalmazza:
 * - a teljes rudak készletét (profileInventory),
 * - a maradék rudak készletét (reusableInventory).
 *
 * A snapshot mindig csak másolat, tehát az optimalizáció
 * szabadon "fogyaszthatja" vagy módosíthatja, a valódi registryt
 * ez nem érinti.
 */
struct InventorySnapshot {
    QVector<StockEntry> profileInventory;         // teljes rudak snapshotja
    QVector<LeftoverStockEntry> reusableInventory; // maradék rudak snapshotja

    bool isEmpty() const {
        return profileInventory.isEmpty() && reusableInventory.isEmpty();
    }
};
