#include "inventoryauditbuilder.h"

#include "materials/registry/material_registry.h"
#include "materials/registry/material_group_registry.h"
#include "stock/registry/stockregistry.h"

InventoryAuditModel InventoryAuditBuilder::build(
    const QVector<Cutting::Plan::Request>& requests,
    const QMap<QUuid,QVector<int>>& lengthsPerMaterial,
    const QMap<QUuid,QVector<int>>& expandedLengths,
    const QMap<QUuid,int>& strandsPerMaterial,
    const InventorySnapshot& snapshot)
{
    InventoryAuditModel model;

    auto findOrCreateRow =
        [&](const QUuid& materialId) -> InventoryAuditRow&

    {
        for (auto& row : model.rows)
        {
            if (row.materialId == materialId)
                return row;
        }

        InventoryAuditRow row;
        row.materialId = materialId;

        if (const MaterialMaster* m =
            MaterialRegistry::instance().findById(materialId))
        {
            row.barcode = m->barcode;
            row.materialName = m->toReportLabel();
        }

        if (const MaterialGroup* g =
            MaterialGroupRegistry::instance().findByMaterialId(materialId))
        {
            row.materialGroup = g->barcode;
        }

        model.rows.push_back(row);
        return model.rows.last();
    };

    auto findRow =
        [&](const QUuid& materialId) -> InventoryAuditRow*
    {
        for (auto& row : model.rows)
        {
            if (row.materialId == materialId)
                return &row;
        }

        return nullptr;
    };

    //----------------------------------------------------
    // 1. REQUEST szint
    //----------------------------------------------------

    for (const auto& req : requests)
    {
        auto& row = findOrCreateRow(req.materialId);

        row.requestPieces += req.quantity;

        row.requestedLengthMm +=
            static_cast<qint64>(req.requiredLength) *
            static_cast<qint64>(req.quantity);
    }

    //----------------------------------------------------
    // 2. LENGTHS szint
    //----------------------------------------------------

    for (auto it = lengthsPerMaterial.begin();
         it != lengthsPerMaterial.end();
         ++it)
    {
        auto& row = findOrCreateRow(it.key());

        row.requestPieces =
            qMax(row.requestPieces,
                 it.value().size());
    }

    //----------------------------------------------------
    // 3. EXPANDED szint
    //----------------------------------------------------

    for (auto it = expandedLengths.begin();
         it != expandedLengths.end();
         ++it)
    {
        auto& row = findOrCreateRow(it.key());

        if (!lengthsPerMaterial.contains(it.key()))
        {
            row.isExpanded = true;
        }
    }

    //----------------------------------------------------
    // 4. GREEDY becslés
    //----------------------------------------------------

    for (auto it = strandsPerMaterial.begin();
         it != strandsPerMaterial.end();
         ++it)
    {
        auto& row = findOrCreateRow(it.key());

        row.estimatedStrands = it.value();
    }

    //----------------------------------------------------
    // 5. STOCK készlet
    //----------------------------------------------------

    QMap<QUuid,int> stockPerMaterial;

    for (const StockEntry& s :
         StockRegistry::instance().readAll())
    {
        stockPerMaterial[s.materialId] += s.quantity;
    }

    for (auto it = stockPerMaterial.begin();
         it != stockPerMaterial.end();
         ++it)
    {
        auto* row = findRow(it.key());

        if(!row) continue;

        row->stockStrands = it.value();
    }

    //----------------------------------------------------
    // 6. SNAPSHOT
    //----------------------------------------------------

    QMap<QUuid,int> snapshotPerMaterial;

    for (const StockEntry& s :
         snapshot.profileInventory)
    {
        snapshotPerMaterial[s.materialId] += s.quantity;
    }

    for (auto it = snapshotPerMaterial.begin();
         it != snapshotPerMaterial.end();
         ++it)
    {
        auto* row = findRow(it.key());

        if(!row) continue;

        row->snapshotStrands = it.value();
    }


    QMap<QString, InventoryAuditGroupSummary> summaries;
    for (const auto& row : model.rows)
    {
        if (row.materialGroup.isEmpty())
            continue;

        auto& s = summaries[row.materialGroup];

        s.groupKey = row.materialGroup;

        if (const MaterialGroup* g =
            MaterialGroupRegistry::instance()
                .findByBarcode(row.materialGroup))
        {
            s.groupName = g->name;
        }
        //s.totalGreedyStrands += row.estimatedStrands;
        s.totalSnapshotStrands += row.snapshotStrands;
        s.totalStockStrands += row.stockStrands;

        if (row.isExpanded)
        {
            s.expandedGreedyStrands += row.estimatedStrands;
        }
        else
        {
            s.originalGreedyStrands += row.estimatedStrands;
        }
    }

    for (auto it = summaries.begin();
         it != summaries.end();
         ++it)
    {
        InventoryAuditGroupSummary& s = it.value();

        s.totalGreedyStrands =
            s.originalGreedyStrands +
            s.expandedGreedyStrands;

        s.hasShortage =
            s.totalSnapshotStrands <
            s.totalGreedyStrands;

        model.groups.push_back(s);
    }
    return model;
}