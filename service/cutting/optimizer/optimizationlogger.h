#ifndef OPTIMIZATIONLOGGER_H
#define OPTIMIZATIONLOGGER_H

#include "../../../common/logger.h"
#include "../../../model/cutting/plan/cutplan.h"
#include "../../../model/cutting/result/resultmodel.h"

#include <materials/model/material_master.h>

#include <materials/registry/material_registry.h>

struct OptimizationLogger {
    static void logPlans(const QVector<Cutting::Plan::CutPlan>& plans,
                         const QVector<Cutting::Result::ResultModel>& results) {
        zInfo(L("✅ VÁGÁSI TERVEK — CutPlan-ek:"));


        for (const auto& plan : plans) {

            QString segmentstxt="";
            for(const auto& p : plan.piecesWithMaterial) {
                if(!segmentstxt.isEmpty())
                    segmentstxt+=", ";
                //const MaterialMaster* mat = MaterialRegistry::instance().findById(p.materialId);
                segmentstxt += QString("%1. %2 mm").arg(p.info.externalReference).arg(p.info.length_mm);
            }

            QString msg = L("   •%1  segments: %2, Hulladék: %3 mm")
                              .arg(plan.rodId)   // 🔑 Stabil rúd azonosító
                              .arg(segmentstxt)
                              .arg(plan.waste);
            zInfo(msg);
        }

        zInfo(L("♻️ KELETKEZETT HULLADÉKOK:"));
        for (const auto& r : results) {
            QString msg = L("   •Hulladék: %1 mm, Barcode: %2")
                              .arg(r.waste)
                              .arg(r.reusableBarcode);
            zInfo(msg);
        }
    }
};

#endif // OPTIMIZATIONLOGGER_H
