#pragma once

#include "model/cutting/plan/cutplan.h"
#include "service/cutting/optimizer/optimizerconstants.h"

#include <materials/model/trimmingparams.h>
namespace Cutting {
namespace Optimizer {
namespace SegmentPostProcess {

inline void applyFrontTrimToPlan(Cutting::Plan::CutPlan& plan, MaterialTrimmingParams tp)
{
    //double kerf_mm
    //bool isStockRod = plan.isReusable();
    zInfo(QString("🔍 FRONT TRIM — planId=%1, isStock=%2")
              .arg(plan.planId.toString())
              .arg(plan.isStockRod()));

    // Csak stock rúdra alkalmazzuk
    if (!plan.isStockRod())
        return;

    // // 1️⃣ Megkeressük a megfelelő CutPlan‑t
    // auto it = std::find_if(_result_plans.begin(), _result_plans.end(),
    //                        [&](const Cutting::Plan::CutPlan& p){
    //                            return p.planId == planId;
    //                        });
    // if (it == _result_plans.end())
    //     return;

    // Cutting::Plan::CutPlan& plan = *it;
    // //auto& segs = plan.segments();
    // if (plan._segments.isEmpty())
    //     return;

    double frontTrim = tp.backTrim_mm; // 15 mm
    double frontKerf = plan.machineKerf;

    double delta = frontTrim + frontKerf;

    // Ha a végmaradék ennél kisebb, nem piszkáljuk (védőfék)
    if (plan._segments.waste_mm() <= delta) {
        zInfo(QString("✖ FRONT TRIM — waste túl kicsi (waste=%1 mm, delta=%2 mm)")
                  .arg(plan._segments.waste_mm())
                  .arg(delta));
        return;
    }

    zInfo(QString("🎯 FRONT TRIM — waste rövidítve delta=%1 mm").arg(delta));

    // 4️⃣ Elejére beszúrjuk: [Technical(15)] + [Kerf(frontKerf)]
    Cutting::Segment::SegmentModel frontTech(
        Cutting::Segment::SegmentModel::Type::Technical,
        frontTrim,
        /*ix*/ 0,
        QUuid(), QUuid(),""
        );

    Cutting::Segment::SegmentModel frontKerfSeg(
        Cutting::Segment::SegmentModel::Type::Kerf,
        frontKerf,
        /*ix*/ 0,
        QUuid(), QUuid(),""
        );

    zInfo(QString("➡ FRONT TRIM — technikai és kerf szegmens beszúrva (trim=%1 mm, kerf=%2 mm)")
              .arg(frontTrim)
              .arg(frontKerf));


    plan._segments.insert(0, frontKerfSeg);
    plan._segments.insert(0, frontTech);

    zInfo("📊 FRONT TRIM — kész, indexek újraszámozva");

    // ⚖️ plan.waste, result.waste, leftover.availableLength_mm NEM változik
    // csak a waste szegmens eloszlását módosítottuk (eleje vs vége),
    // az össz‑hossz változatlan.

}

}// namespace SegmentPostProcess
} // namespace Optimizer
} // namespace Cutting
