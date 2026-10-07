#pragma once

#include "inventoryauditmodel.h"

#include <model/cutting/plan/request.h>

#include <model/inventorysnapshot.h>

class InventoryAuditBuilder
{
public:

    static InventoryAuditModel build(
        const QVector<Cutting::Plan::Request>& requests,
        const QMap<QUuid,QVector<int>>& lengthsPerMaterial,
        const QMap<QUuid,QVector<int>>& expandedLengths,
        const QMap<QUuid,int>& strandsPerMaterial,
        const InventorySnapshot& snapshot);
};