#pragma once

#include "../../storageaudit/storageauditservice.h"
#include "../../storageaudit/leftoverauditservice.h"
#include "../../storageaudit/auditutils.h"
#include "../../../model/storageaudit/storageauditrow.h"
#include "../../../model/cutting/plan/cutplan.h"
#include "../../../model/cutting/optimizer/optimizermodel.h"

struct OptimizationAuditBuilder {
    static QVector<StorageAuditRow> build(Cutting::Optimizer::OptimizerModel& model) {
        auto& plans = model.getResult_PlansRef();

        // 📥 Audit sorok legenerálása
        QVector<StorageAuditRow> stockAuditRows = StorageAuditService::generateAuditRows_All();
        QVector<StorageAuditRow> leftoverAuditRows = LeftoverAuditService::generateAuditRows_All();

        QVector<StorageAuditRow> all = stockAuditRows + leftoverAuditRows;

        // 🧩 Injektálás
        QMap<QUuid, int> pickingMap = generatePickingMapFromPlans(plans);
        AuditUtils::injectPlansIntoAuditRows(plans, &all);
        AuditUtils::assignContextsToRows(&all, pickingMap);

        return all;
    }

private:
    static QMap<QUuid, int> generatePickingMapFromPlans(const QVector<Cutting::Plan::CutPlan>& plans) {
        QMap<QUuid, int> pickingMap;
        for (const auto& plan : plans) {
            if (plan.isReusable()) continue;
            pickingMap[plan.materialId] += 1;
        }
        return pickingMap;
    }
};

