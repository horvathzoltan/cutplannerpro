#pragma once

#include "common/logger.h"
#include "model/cutting/plan/cutplan.h"
#include "model/relocation/relocationinstruction.h"

#include <QMap>

#include <model/registries/storageregistry.h>

namespace RelocationPlanner {


inline QVector<RelocationInstruction> buildPlan(
    const QVector<Cutting::Plan::CutPlan>& cutPlans,
    const QVector<StorageAuditRow>& auditRows)
{
    QVector<RelocationInstruction> plan;

    // 1️⃣ Auditálatlan sorok figyelmeztetése
    const bool hasUnaudited = std::any_of(auditRows.begin(), auditRows.end(),
                                          [](const StorageAuditRow& row) {
                                              return row.wasModified && !row.isAuditConfirmed;
                                          });
    if (hasUnaudited) {
        zWarning("⚠️ Auditálatlan sorok találhatók – a relocation terv nem teljesen megbízható!");
    }

    // 2️⃣ CutPlan-ek szétválogatása
    QMap<QUuid, int> requiredStockByMaterial;
    QMap<QUuid, QString> materialCodeById;
    QMap<QUuid, QString> materialNameById;
    QVector<const Cutting::Plan::CutPlan*> reusablePlans;

    for (const auto& planItem : cutPlans) {
        if (planItem.source == Cutting::Plan::Source::Stock) {
            requiredStockByMaterial[planItem.materialId] += 1;
            materialCodeById[planItem.materialId] = planItem.materialBarcode();
            materialNameById[planItem.materialId] = planItem.materialName();
        } else if (planItem.source == Cutting::Plan::Source::Reusable) {
            reusablePlans.append(&planItem);
        }
    }

    // 3️⃣ Stock anyagok relocation terve
    for (auto it = requiredStockByMaterial.begin(); it != requiredStockByMaterial.end(); ++it) {
        const QUuid materialId = it.key();
        const int requiredQty = it.value();
        const QString materialCode = materialCodeById.value(materialId);
        const QString materialName = materialNameById.value(materialId);

        // 🔍 Root tároló meghatározása audit alapján
        // Ez a "gépszintű" gyökér, amely alá a célhelyek tartoznak.
        QUuid rootId;
        auto rowIt = std::find_if(auditRows.begin(), auditRows.end(),
                                  [&](const StorageAuditRow& r){ return r.materialId == materialId; });
        if (rowIt != auditRows.end())
            rootId = rowIt->rootStorageId;

        // 🎯 Célhelyek listázása (rootId alatti storage-ok)
        const auto targetEntries = StorageRegistry::instance().getRecursive(rootId);
        QVector<RelocationTargetEntry> targetList;
        for (const auto& entry : targetEntries) {
            RelocationTargetEntry tgt;
            tgt.locationId = entry.id;
            tgt.locationName = entry.name;
            tgt.placed = 0;
            targetList.append(tgt);
        }

        // 📦 Források gyűjtése a StockRegistry-ből
        // Fontos: nem az audit sorokból, hanem a tényleges készletből dolgozunk.
        QVector<RelocationSourceEntry> sourceList;
        auto stockEntries = StockRegistry::instance().findByMaterialId(materialId);

        for (const auto& entry : stockEntries) {
            // 🚫 Kizárjuk a célhelyeket  (vágóhelyek - ne legyen "forrás == cél")
            if (StorageRegistry::instance().isDescendantOf(entry.storageId, rootId)) continue;

            RelocationSourceEntry src;
            src.locationId   = entry.storageId;
            src.locationName = StorageRegistry::instance().findById(entry.storageId)->name;
            src.available    = entry.quantity;
            src.moved        = 0;
            sourceList.append(src);
        }

        // 📊 Mennyi van már a célhelyen → ebből számoljuk a hiányt
        int presentAtTarget = 0;
        int auditedAtTarget = 0;
        for (const auto& row : auditRows) {
            if (row.sourceType != AuditSourceType::Stock) continue;
            if (row.materialId != materialId) continue;
            if (row.rootStorageId != rootId) continue;

            presentAtTarget += row.actualQuantity;
            if (row.isAuditConfirmed)
                auditedAtTarget += row.actualQuantity;
        }

        // ➡️ Mozgatandó mennyiség = igény – ami már ott van
        int toMove = std::max(0, requiredQty - presentAtTarget);

        // 📋 RelocationInstruction építése (egyedi sor a dialógushoz)
        RelocationInstruction instr(materialName,
                                    toMove,  // 🔹 nem a teljes igény, hanem a hiány
                                    false,
                                    materialCode,
                                    AuditSourceType::Stock,
                                    materialId);
        instr.sources = sourceList;
        instr.targets = targetList;
        //instr.dialogTotalMovedQuantity = 0;
        instr.finalizedQuantity = std::nullopt; // még nincs finalize-olva;
        plan.append(instr);

        // 📊 Összesítő sor
        int totalRemaining = 0;
        int auditedRemaining = 0;
        for (const auto& row : auditRows) {
            if (row.materialId != materialId) continue;
            totalRemaining += row.actualQuantity;
            if (row.isAuditConfirmed)
                auditedRemaining += row.actualQuantity;
        }



        int usedFromRemaining = std::min(requiredQty, presentAtTarget);
        int coveredQty = std::min(requiredQty, totalRemaining);
        int uncoveredQty = std::max(0, requiredQty - coveredQty);

        QString status;
        if (uncoveredQty == 0) {
            status = auditedAtTarget < presentAtTarget
                         ? "🟡 Részlegesen auditált, ✔ Igény teljesítve"
                         : "🟢 Teljesen auditált, ✔ Igény teljesítve";
        } else {
            status = QString("🔴 Nem teljesített, Lefedetlen: %1").arg(uncoveredQty);
        }

        RelocationInstruction summary(materialName,
                                      requiredQty,
                                      totalRemaining,
                                      auditedRemaining,
                                      0,
                                      uncoveredQty,
                                      coveredQty,
                                      usedFromRemaining,
                                      status,
                                      materialCode,
                                      AuditSourceType::Stock,
                                      materialId);
        plan.append(summary);
    }

    // 4️⃣ Hullók (Reusable)
    for (const auto* planItem : reusablePlans) {
        const QUuid materialId   = planItem->materialId;
        const QString materialName = planItem->materialName();
        const QString rodBarcode   = planItem->rodId;

        auto it = std::find_if(auditRows.begin(), auditRows.end(), [&](const StorageAuditRow& row) {
            return row.isAuditConfirmed &&
                   row.sourceType == AuditSourceType::Leftover &&
                   row.materialId == materialId &&
                   row.barcode == rodBarcode;
        });

        if (it != auditRows.end()) {
            RelocationInstruction instr(materialName,
                                        0,      // nincs igény darabszám
                                        true,   // jelen van → Megvan
                                        rodBarcode,
                                        AuditSourceType::Leftover,
                                        materialId);

            // 📦 Forrás: jelezzük, hogy a hulló a saját helyén van
            RelocationSourceEntry src;
            src.locationId   = it->storageId();   // auditRow-ból lekérdezve
            src.locationName = it->storageName;   // auditRow-ból
            src.available    = it->actualQuantity;
            src.moved        = 0;                 // nem mozgatjuk
            instr.sources.append(src);

            //instr.dialogTotalMovedQuantity = 0;
            instr.finalizedQuantity = std::nullopt;
            plan.push_back(instr);
        }
    }


    return plan;
}
} // namespace RelocationPlanner
