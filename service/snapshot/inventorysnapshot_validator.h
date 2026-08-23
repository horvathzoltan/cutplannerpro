#pragma once

#include "common/validation_result.h"
#include "model/inventorysnapshot.h"
#include "materials/registry/material_group_registry.h"

namespace InventorySnapshotValidator{

inline ValidationResult validate( const InventorySnapshot& inventory,
                                 const QMap<QUuid, int>& strandsPerMaterial){
    ValidationResult result;
    // 5/b️⃣ Inventory sanity check
    if (inventory.profileInventory.isEmpty()){
        result.errors << "Az inventory üres – nincs felhasználható szál.";
        return result;
    }

    // 6️⃣ Group‑szintű igény és készlet aggregálása
    QMap<QUuid, int> needPerMaterial = strandsPerMaterial;
    QMap<QUuid, int> havePerMaterial;

    for (const StockEntry& s : inventory.profileInventory)
        havePerMaterial[s.materialId] += s.quantity;

    struct GroupTotals { int need = 0; int have = 0; };
    QMap<QUuid, GroupTotals> groupTotals;

    for (auto it = needPerMaterial.begin(); it != needPerMaterial.end(); ++it) {
        QUuid matId = it.key();
        int need = it.value();
        const MaterialGroup* g = MaterialGroupRegistry::instance().findByMaterialId(matId);
        QUuid groupId = g ? g->id : matId;
        groupTotals[groupId].need += need;
    }

    for (auto it = havePerMaterial.begin(); it != havePerMaterial.end(); ++it) {
        QUuid matId = it.key();
        int have = it.value();
        const MaterialGroup* g = MaterialGroupRegistry::instance().findByMaterialId(matId);
        QUuid groupId = g ? g->id : matId;
        groupTotals[groupId].have += have;
    }

    // 7️⃣ Group‑szintű hiányellenőrzés
    for (auto it = groupTotals.begin(); it != groupTotals.end(); ++it) {
        const GroupTotals& totals = it.value();
        if (totals.need == 0)
            continue;

        if (totals.have < totals.need) {
            const MaterialGroup* g = MaterialGroupRegistry::instance().findById(it.key());
            QString groupName = g ? g->name : QString("Ismeretlen csoport");
            result.warnings << QString("Kevés készlet az anyagcsoportban: %1 (kell: %2 szál, van: %3 szál)")
                                   .arg(groupName)
                                   .arg(totals.need)
                                   .arg(totals.have);
        }
    }

    return result;
}


}