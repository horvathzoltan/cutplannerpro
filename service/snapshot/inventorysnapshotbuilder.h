#pragma once

#include "model/inventorysnapshot.h"
#include <model/registries/cuttingplanrequestregistry.h>
#include <model/registries/leftoverstockregistry.h>
#include <model/registries/stockregistry.h>

#include <model/cutting/plan/request.h>

/**
 * @brief Service osztály, amely InventorySnapshot-ot épít a registrykből.
 *
 * Ez a réteg választja le az optimizert az éles registrykről:
 * - az optimizer csak a snapshotot kapja,
 * - a registrykhez való hozzáférés itt történik.
 *
 * Nem végez validációt, nem mutat hibát, csak adatot ad vissza.
 * A validáció és a hibakezelés a presenter feladata.
 */

class InventorySnapshotBuilder {
public:
    /// Teljes készlet snapshot (stock + reusable)
    static InventorySnapshot build(int leftoverMinLength) {
        InventorySnapshot snapshot;
        snapshot.profileInventory = StockRegistry::instance().readAll();
        snapshot.reusableInventory = LeftoverStockRegistry::instance().filtered(leftoverMinLength);
        return snapshot;
    }
};
