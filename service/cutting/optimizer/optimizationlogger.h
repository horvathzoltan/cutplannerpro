#pragma once

#include "../../../common/logger.h"
#include "../../../model/cutting/plan/cutplan.h"
#include "model/cutting/plan/parentinfo.h"

#include <materials/model/material_master.h>

#include <materials/registry/material_registry.h>

struct OptimizationLogger {
    static void logPlans(const QVector<Cutting::Plan::CutPlan>& plans)
    {
        zInfo(L("✅ VÁGÁSI TERVEK — részletes audit log:"));

        for (const auto& plan : plans) {

            // 1) PLAN FEJLÉC
            zInfo(QString("📘 PLAN #%1 —> planId=%3")
                      .arg(plan.planNumber)
                      .arg(plan.planId.toString(QUuid::WithoutBraces)));

            zInfo(QString("   • optimizationId=%1").arg(plan.optimizationId));
            zInfo(QString("   • rodId=%1").arg(plan.rodId));
            zInfo(QString("   • materialName=%1").arg(plan.materialName()));
            zInfo(QString("   • totalKerfLength=%1 mm").arg(plan._segments.kerfInfo().length));
            zInfo(QString("   • status=%1").arg(statusText(plan.status)));
            zInfo(QString("   • Source: %1, sourceBarcode=%2")
                      .arg(plan.source == Cutting::Plan::Source::Stock ? "Stock" : "Reusable")
                      .arg(plan.sourceBarcode));

            // PATCH 9 — ParentInfo audit‑minőségű kiírása
            if (!plan.parent().has_value()) {
                zInfo("   • Parent: ROOT (stock)");
            } else {
                zInfo(QString("   • Parent: %1").arg(plan.parent()->toString()));

                // const auto& parent = plan.parent().value();

                // if (!parent.planId().has_value()) {
                //     // stock gyökér (csak barcode ismert)
                //     zInfo(QString("   • Parent: %1 (stock)")
                //               .arg(parent.barcode()));
                // } else {
                //     // leftover → plan lánc
                //     zInfo(QString("   • Parent: %1 (plan=%2)")
                //               .arg(parent.barcode())
                //               .arg(parent.planId(->toString(QUuid::WithoutBraces)));
                // }
            }


            zInfo(QString("   • machineName: %1, machineKerf=%2 mm")
                      .arg(plan.machineName)
                      .arg(plan.machineKerf));

            // 2) DARABOK LISTÁJA
            zInfo(L("   ✂️ Levágott darabok (piecesWithMaterial):"));
            for (const auto& p : plan.piecesWithMaterial) {
                zInfo(QString("      • %1. %2 mm  (pieceId=%3)")
                          .arg(p.info.externalReference)
                          .arg(p.info.length_mm)
                          .arg(p.info.pieceId.toString(QUuid::WithoutBraces)));
            }

            // 3) SZEGMENSEK LISTÁJA
            // 3) SZEGMENSEK LISTÁJA
            zInfo(L("   🧱 Szakaszok (segments):"));
            double segSum_mm = 0.0;

            //for (const auto& s : plan.segments())
            for(int segIx = 0; segIx < plan._segments.segments().size(); ++segIx) {
                const auto& s = plan._segments.segment(segIx);

                segSum_mm += s.length_mm();

                QString type;
                if (s.isPiece())      type = "Piece";
                else if (s.isKerf())  type = "Kerf";
                else if (s.isTechnical())  type = "Technical";
                //else if (s.isWaste()) type = "Waste";
                else                  type = "Unknown";

                zInfo(QString("      • [%1] len=%2 mm  (segIx=%3, barcode=%4)")
                          .arg(type)
                          .arg(s.length_mm())
                          .arg(segIx)
                          .arg(s.barcode()));
            }

            double used  = segSum_mm;
            double waste = plan._segments.waste_mm();
            double total = plan._segments.totalLength_mm();

            zInfo(QString("   • TotalLength=%1 mm, segmentsSum=%2 mm, Waste=%3 mm")
                      .arg(total)
                      .arg(segSum_mm, 0, 'f', 1)
                      .arg(waste));

            double diff = (used + waste) - total;

            zInfo(QString("   • SegmentSumCheck: used+Waste-Total = %1 mm")
                      .arg(diff, 0, 'f', 3));

            // 4) HULLÓ BLOKK
            zInfo(L("   ♻️ Hulló blokk:"));
            zInfo(QString("   • plan._segments.leftoverBarcode=%1").arg(plan._segments.leftoverBarcode()));
            zInfo(QString("   • plan._segments.waste=%1 mm").arg(plan._segments.waste_mm()));
        }

        // A külön „KELETKEZETT HULLADÉKOK” szekciót eltávolítjuk → redundáns
    }

};


