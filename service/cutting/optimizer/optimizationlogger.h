#ifndef OPTIMIZATIONLOGGER_H
#define OPTIMIZATIONLOGGER_H

#include "common/logger.h"
#include "model/cutting/plan/cutplan.h"
#include "model/cutting/result/resultmodel.h"

struct OptimizationLogger {
    static void logPlans(const QVector<Cutting::Plan::CutPlan>& plans,
                         const QVector<Cutting::Result::ResultModel>& results) {
        zInfo(L("✅ VÁGÁSI TERVEK — CutPlan-ek:"));
        for (const auto& plan : plans) {
            QString msg = L("  → #%1 | PlanId: %2 | Vágások: %3 | Hulladék: %4 mm")
                              .arg(plan.rodNumber)
                              .arg(plan.planId.toString())
                              .arg(plan.piecesWithMaterial.size())
                              .arg(plan.waste);
            zInfo(msg);
        }

        zInfo(L("♻️ KELETKEZETT HULLADÉKOK:"));
        for (const auto& r : results) {
            QString msg = L("  - Hulladék: %1 mm | Barcode: %2")
                              .arg(r.waste)
                              .arg(r.reusableBarcode);
            zInfo(msg);
        }
    }
};

#endif // OPTIMIZATIONLOGGER_H
