#ifndef OPTIMIZATIONLOGGER_H
#define OPTIMIZATIONLOGGER_H

#include "../../../common/logger.h"
#include "../../../model/cutting/plan/cutplan.h"
#include "../../../model/cutting/result/resultmodel.h"

#include <materials/model/material_master.h>

#include <materials/registry/material_registry.h>

struct OptimizationLogger {
    static void logPlans(const QVector<Cutting::Plan::CutPlan>& plans,
                         const QVector<Cutting::Result::ResultModel>& results)
    {
        zInfo(L("✅ VÁGÁSI TERVEK — részletes audit log:"));

        for (const auto& plan : plans) {

            // 1) PLAN FEJLÉC
            zInfo(QString("📘 PLAN #%1 — rodId=%2, planId=%3")
                      .arg(plan.planNumber)
                      .arg(plan.rodId)
                      .arg(plan.planId.toString(QUuid::WithoutBraces)));

            zInfo(QString("   • materialId=%1").arg(plan.materialId.toString(QUuid::WithoutBraces)));
            zInfo(QString("   • sourceBarcode=%1").arg(plan.sourceBarcode));
            zInfo(QString("   • parentBarcode=%1").arg(plan.parentBarcode.has_value() ? plan.parentBarcode.value() : "—"));
            zInfo(QString("   • parentPlanId=%1").arg(plan.parentPlanId.has_value()
                                                          ? plan.parentPlanId.value().toString(QUuid::WithoutBraces)
                                                          : "—"));
            zInfo(QString("   • leftoverBarcode=%1").arg(plan.leftoverBarcode));
            zInfo(QString("   • kerfTotal=%1 mm").arg(plan.kerfTotal));
            zInfo(QString("   • kerfUsed_mm=%1 mm").arg(plan.kerfUsed_mm));
            zInfo(QString("   • totalLength=%1 mm").arg(plan.totalLength));
            zInfo(QString("   • optimizationId=%1").arg(plan.optimizationId));
            zInfo(QString("   • status=%1").arg(static_cast<int>(plan.status)));


            zInfo(QString("   • Source: %1  (Barcode=%2)")
                      .arg(plan.source == Cutting::Plan::Source::Stock ? "Stock" : "Reusable")
                      .arg(plan.sourceBarcode));

            if (plan.parentBarcode.has_value())
                zInfo(QString("   • Parent: barcode=%1, planId=%2")
                          .arg(plan.parentBarcode.value())
                          .arg(plan.parentPlanId.has_value()
                                   ? plan.parentPlanId.value().toString(QUuid::WithoutBraces)
                                   : "—"));

            zInfo(QString("   • Machine: %1  kerf=%2 mm")
                      .arg(plan.machineName)
                      .arg(plan.kerfUsed_mm));

            zInfo(QString("   • TotalLength: %1 mm  Waste: %2 mm")
                      .arg(plan.totalLength)
                      .arg(plan.waste));

            // 2) DARABOK LISTÁJA
            zInfo(L("   ✂️ Levágott darabok:"));
            for (const auto& p : plan.piecesWithMaterial) {
                zInfo(QString("      • %1. %2 mm  (pieceId=%3)")
                          .arg(p.info.externalReference)
                          .arg(p.info.length_mm)
                          .arg(p.info.pieceId.toString(QUuid::WithoutBraces)));
            }

            // 3) SZEGMENSEK LISTÁJA
            zInfo(L("   🧱 Szakaszok (segments):"));
            for (const auto& s : plan.segments) {
                QString type;
                if (s.isPiece())      type = "Piece";
                else if (s.isKerf())  type = "Kerf";
                else if (s.isTechnical())  type = "Technical";
                else if (s.isWaste()) type = "Waste";
                else                  type = "Unknown";

                zInfo(QString("      • [%1] len=%2 mm  (segIx=%3, barcode=%4)")
                          .arg(type)
                          .arg(s.length_mm())
                          .arg(s._segIx)
                          .arg(s.barcode()));
            }

            // 4) HULLÓ BLOKK
            zInfo(QString("   • result.cutPlanId=%1").arg(plan.planId.toString(QUuid::WithoutBraces)));
            zInfo(QString("   • result.sourceBarcode=%1").arg(plan.sourceBarcode));
            zInfo(QString("   • result.parentBarcode=%1").arg(plan.parentBarcode.has_value() ? plan.parentBarcode.value() : "—"));
            zInfo(QString("   • result.leftoverBarcode=%1").arg(plan.leftoverBarcode));
            zInfo(QString("   • result.waste=%1 mm").arg(plan.waste));

            zInfo(L("   ♻️ Hulló:"));
            zInfo(QString("      • leftover=%1 mm, barcode=%2")
                      .arg(plan.waste)
                      .arg(plan.leftoverBarcode));

        }

        // A külön „KELETKEZETT HULLADÉKOK” szekciót eltávolítjuk → redundáns
    }

};

#endif // OPTIMIZATIONLOGGER_H
