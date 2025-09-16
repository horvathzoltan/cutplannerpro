#pragma once

#include "common/logger.h"
#include "model/cutting/plan/cutplan.h"
#include <model/storageaudit/storageauditrow.h>

#include <QMap>
namespace AuditUtils {

void injectPlansIntoAuditRows(const QVector<Cutting::Plan::CutPlan>& plans,
                              QVector<StorageAuditRow>* auditRows)
{
    if (!auditRows)
        return;

    // Összesített anyagigény stock esetén
    QMap<QUuid, int> requiredStockMaterials;

    for (const auto& plan : plans) {
        if (plan.source == Cutting::Plan::Source::Stock)
            requiredStockMaterials[plan.materialId]++;
    }

    for (auto& row : *auditRows) {
        if (row.sourceType == AuditSourceType::Leftover) {
            // Hulló audit: barcode alapján összevetés
            for (const auto& plan : plans) {
                if (plan.source == Cutting::Plan::Source::Reusable &&
                    plan.rodId == row.barcode) {
                    row.isInOptimization = true;
                    break;
                }
            }
        } else {
            // Stock audit: materialId alapján összevetés
            if (requiredStockMaterials.contains(row.materialId)) {
                row.isInOptimization = true;
                row.pickingQuantity = requiredStockMaterials.value(row.materialId);
            }
        }
    }

    zInfo(L("🔄 Audit sorok frissítve a vágási terv alapján — összes sor: %1").arg(auditRows->size()));
}

} // namespace AuditUtils
