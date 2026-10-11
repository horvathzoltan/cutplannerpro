#include "inventoryauditbuilder.h"

#include "materials/registry/material_registry.h"
#include "materials/registry/material_group_registry.h"
#include "stock/registry/stockregistry.h"
#include "materialbundles/registry/bundle_registry.h"

InventoryAuditModel InventoryAuditBuilder::build(
    const QVector<Cutting::Plan::Request>& requests,
    const QMap<QUuid,QVector<int>>& lengthsPerMaterial,
    const QMap<QUuid,
               RequestSnapshotBuilder::MaterialLengthDemand>& expandedLengths,
    const QMap<QUuid,
               InventorySnapshotBuilder::StrandDemandEstimate>& strandsPerMaterial,
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
        const auto& demand = it.value();

        auto& row =
            findOrCreateRow(demand.materialId);

        row.originMaterialId =
            demand.originMaterialId;

        row.isExpanded =
            demand.origin ==
            RequestSnapshotBuilder::MaterialDemandOrigin::Expanded;

        if (row.isExpanded)
        {
            if (const MaterialMaster* originMat =
                MaterialRegistry::instance()
                    .findById(demand.originMaterialId))
            {
                row.sourceBarcode =
                    originMat->barcode;
            }
        }
    }

    //----------------------------------------------------
    // 4. GREEDY becslés
    //----------------------------------------------------

    for (auto it = strandsPerMaterial.begin();
         it != strandsPerMaterial.end();
         ++it)
    {
        const auto& estimate =
            it.value();

        auto& row =
            findOrCreateRow(estimate.materialId);

        row.estimatedStrands =
            estimate.strandCount;
    }

    //----------------------------------------------------
    // 5. STOCK készlet
    //----------------------------------------------------

    QMap<QUuid,int> stockPerMaterial =
        StockRegistry::instance().readAllAggregated();

    for (auto it = stockPerMaterial.begin();
         it != stockPerMaterial.end();
         ++it)
    {
        auto* row = findRow(it.key());

        if(!row) continue;

        row->stockStrands = it.value();
    }

    //----------------------------------------------------
    // 5/b ASSEMBLABLE bundle készlet
    //----------------------------------------------------

    for (auto& row : model.rows)
    {
        const MaterialMaster* master =
            MaterialRegistry::instance()
                .findById(row.materialId);

        if (!master)
            continue;

        if (master->kind != MaterialKind::Bundle)
            continue;

        row.componentAssemblyStock =
            BundleRegistry::instance()
                .computeAssemblableCount(
                    master->bundleCode,
                    stockPerMaterial);
        const auto components =
            BundleRegistry::instance()
                .componentsOf(
                    master->bundleCode);

        for (const BundleComponent& comp : components)
        {
            InventoryAuditComponent item;

            item.materialId =
                comp.materialId;

            item.requiredPerBundle =
                comp.count;

            item.stock =
                stockPerMaterial.value(
                    comp.materialId,
                    0);

            item.assemblableCount =
                comp.count > 0
                    ? item.stock / comp.count
                    : 0;

            if (const MaterialMaster* cm =
                MaterialRegistry::instance()
                    .findById(comp.materialId))
            {
                item.barcode =
                    cm->barcode;

                item.materialName =
                    cm->toReportLabel();
            }

            row.assemblyComponents
                .push_back(item);
        }
    }

    //----------------------------------------------------
    // 5/c AVAILABLE stock
    //----------------------------------------------------

    for (auto& row : model.rows)
    {

        //----------------------------------------------------
        // TODO:
        // availableStock currently assumes that
        // stockStrands and componentAssemblyStock
        // are disjoint sources.
        //
        // This must be verified before coverage
        // fully relies on availableStock.
        //----------------------------------------------------

        row.availableStock =
            row.stockStrands +
            row.componentAssemblyStock;
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

    //----------------------------------------------------
    // 7. summary
    //----------------------------------------------------

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
        if (row.isExpanded)
        {
            s.alternativeAvailable += row.availableStock;

            s.alternativeEstimate += row.estimatedStrands;
        }
        else
        {
            s.requiredDemand =
                qMax(
                    s.requiredDemand,
                    row.estimatedStrands);

            // NOTE:
            //
            // originalAvailable currently aggregates
            // all REQ rows in the group.
            //
            // This assumes independent inventories.
            // Verify if multiple REQ rows can draw
            // from the same physical stock.

            s.originalAvailable += row.availableStock;

            s.originalEstimate += row.estimatedStrands;
        }
    }


    for (auto it = summaries.begin();
         it != summaries.end();
         ++it)
    {
        InventoryAuditGroupSummary& s = it.value();

        s.totalAvailable =
            s.originalAvailable +
            s.alternativeAvailable;

        if (s.originalAvailable >= s.requiredDemand)
        {
            s.coverage =
                GroupCoverage::Original;
        }
        else if (
            s.originalAvailable > 0 &&
            s.totalAvailable >= s.requiredDemand)
        {
            s.coverage =
                GroupCoverage::Partial;
        }
        else if (
            s.originalAvailable == 0 &&
            s.alternativeAvailable >= s.requiredDemand)
        {
            s.coverage =
                GroupCoverage::Substitute;
        }
        else
        {
            s.coverage =
                GroupCoverage::Shortage;
        }

        model.groups.push_back(s);    }
    return model;
}