#pragma once

#include "inventoryauditmodel.h"
#include "inventorysnapshotbuilder.h"
#include "requestsnapshotbuilder.h"

#include <model/cutting/plan/request.h>

#include <model/inventorysnapshot.h>

class InventoryAuditBuilder
{
public:

    static InventoryAuditModel build(
        const QVector<Cutting::Plan::Request>& requests,
        const QMap<QUuid,QVector<int>>& lengthsPerMaterial,
        const QMap<QUuid,
                   RequestSnapshotBuilder::MaterialLengthDemand>& expandedLengths,
        const QMap<QUuid,
                   InventorySnapshotBuilder::StrandDemandEstimate>& strandsPerMaterial,
        const InventorySnapshot& snapshot);
};