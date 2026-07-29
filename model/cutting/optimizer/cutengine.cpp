#include "cutengine.h"
#include "common/eventlogger.h"
#include "model/cutting/result/resultmodel.h"
#include "service/cutting/optimizer/optimizerutils.h"

CutResult CutEngine::cutSingle(
    const Cutting::Piece::PieceWithMaterial& piece,
    int remainingLength,
    const SelectedRod& rod,
    const CuttingMachine& machine,
    int currentOpId,
    int rodId,
    double kerf_mm,
    int dpLimit,
    int& planCounter)
{
    zInfo(QString("🔍 CUT SINGLE — piece=%1 mm, remaining=%2 mm, rodId=%3")
              .arg(piece.info.length_mm)
              .arg(remainingLength)
              .arg(rod.rodId));

    CutResult cr;
    cr.status = CutResultStatus::Unknown;
    cr.planId = QUuid();
    cr.used = 0;
    cr.waste = 0;

    // PATCH: ToldasEngine fődarab kezelése (keepWhole)
    // Ha a darab egy teljes rúd (ToldasEngine fődarab), akkor nem vágjuk.
    // Nem generálunk CutPlan-t, nem tesszük rúdra, csak jelezzük, hogy nincs vágás.

    // PATCH 9/B — ToldasEngine fődarab kezelése (keepWhole)
    // Ha a darab egy teljes rúd (ToldasEngine fődarab), akkor nem vágjuk,
    // de a CuttingPlan-be be kell kerülnie mint "kész darab".

    // PATCH T1 — ToldasEngine fődarab: a teljes leftover rudat egyben átvesszük, nem vágjuk
    // if (piece.info.keepWhole) {

    //     zInfo(QString("⚠ CUT SINGLE — keepWhole=true → teljes leftover rúd egyben felhasználva (len=%1 mm)")
    //               .arg(piece.info.length_mm));

    //     Cutting::Plan::CutPlan p;
    //     p.planNumber = planCounter++;
    //     p.piecesWithMaterial = { piece };
    //     p.toldasRole = piece.info.toldasRole;
    //     p.materialId = piece.materialId;

    //     // A fődarab leftoverből jön → jelöljük Reusable-ként
    //     p.source = Cutting::Plan::Source::Reusable;
    //     p.rodId = rod.rodId;

    //     //p.rodId = QString("leftover_%1").arg(piece.info.leftoverEntryId.toString());
    //     p.planId = QUuid::createUuid();
    //     p.status = Cutting::Plan::Status::NotStarted;
    //     p.machineId = machine.id;
    //     p.machineName = machine.name;
    //     p.machineKerf = kerf_mm;

    //     // Nincs vágás
    //     p._segments.clear();
    //     p._segments.SetTotalLength_mm(piece.info.length_mm);

    //     // ParentInfo öröklése
    //     if (rod._parent.has_value())
    //         p.setParent(*rod._parent);
    //     else
    //         p.setParent(Cutting::Plan::ParentInfo{ rod.barcode, std::nullopt });

    //     p.sourceBarcode = rod.barcode;
    //     p.optimizationId = currentOpId;

    //     // ResultModel — nincs vágás, nincs hulladék
    //     Cutting::Result::ResultModel result;
    //     result.cutPlanId = p.planId;
    //     result.materialId = piece.materialId;
    //     result.length = piece.info.length_mm;
    //     result.cuts = {};
    //     result.waste = 0;
    //     result.source = Cutting::Result::ResultSource::FromReusable;
    //     result.optimizationId = std::nullopt;
    //     result.reusableBarcode = rod.barcode;
    //     result.isFinalWaste = false;
    //     result._parent = p.parent();
    //     result.sourceBarcode = p.sourceBarcode;

    //     cr.status = CutResultStatus::Ok;
    //     cr.planId = p.planId;
    //     cr.used = 0;
    //     cr.waste = 0;
    //     cr.plan = p;
    //     cr.result = result;
    //     cr.usedPieceIds = { piece.info.pieceId };
    //     cr.leftoverBarcode = rod.barcode;

    //     return cr;
    // }



    //int used = piece.info.length_mm + OptimizerUtils::roundKerfLoss_1(1, kerf_mm);
    auto info = OptimizerUtils::computePhysicalCut({ piece }, kerf_mm, remainingLength);

    double usedNoKerf    = info.totalCut;   // darabhossz
    double usedPhysical  = info.used;       // darabhossz + kerf

    // 1) DP-limit ellenőrzés — kerf nélkül
    if (usedNoKerf > dpLimit) {
        zInfo(QString("✖ CUT SINGLE — DP túllépés (usedNoKerf=%1 > dpLimit=%2)")
                  .arg(usedNoKerf)
                  .arg(dpLimit));
        cr.status = CutResultStatus::Overfill;
        return cr;
    }

    // 2) Fizikai Overfill — kerf-fel
    if (usedPhysical > remainingLength) {
        zInfo(QString("✖ CUT SINGLE — fizikai túlvágás (usedPhysical=%1 > remaining=%2)")
                  .arg(usedPhysical)
                  .arg(remainingLength));
        cr.status = CutResultStatus::Overfill;
        return cr;
    }

    double waste = remainingLength - usedPhysical;


    //t waste = remainingLength - used;

    Cutting::Plan::CutPlan p;
    p.planNumber = planCounter++;
    p.piecesWithMaterial = { piece };
    // PATCH 13 — toldat szerepkör átvezetése
    p.toldasRole = piece.info.toldasRole;

    p.materialId = rod.materialId;
    p.rodId = rod.rodId;
    p.source = rod.isReusable ? Cutting::Plan::Source::Reusable : Cutting::Plan::Source::Stock;
    p.planId = QUuid::createUuid();
    p.status = Cutting::Plan::Status::NotStarted;
    p.machineId = machine.id;
    p.machineName = machine.name;
    p.machineKerf = kerf_mm;

    //int physicalLength = remainingLength;


    int physicalLength = (rod.origin == RodOrigin::Continuation) ? dpLimit : remainingLength;
    p._segments.generateSegments({piece}, kerf_mm, physicalLength);
    p._segments.SetTotalLength_mm(physicalLength);

    // if(rod.barcode=="1")
    //     zInfo("egy");
    p.sourceBarcode = rod.barcode;
    p.optimizationId = currentOpId;

    if (rod._parent.has_value()) {
        p.setParent(*rod._parent);
    } else {
        p.setParent(Cutting::Plan::ParentInfo{ rod.barcode, std::nullopt});
    }

    //p._segments.SetTotalLength_mm(remainingLength_before_cut);

    Cutting::Result::ResultModel result;
    result.cutPlanId = p.planId;
    result.materialId = rod.materialId;
    result.length = remainingLength; // PATCH 1-gyel összhangban
    result.cuts = { piece };
    result.waste = waste;
    result.source = rod.isReusable
                        ? Cutting::Result::ResultSource::FromReusable
                        : Cutting::Result::ResultSource::FromStock;
    result.optimizationId = rod.isReusable ? std::nullopt : std::make_optional(currentOpId);

    result.reusableBarcode = p._segments.leftoverBarcode();
    result.isFinalWaste = (waste <= 0);
    result._parent = p.parent(); // ParentInfo öröklése
    result.sourceBarcode = p.sourceBarcode;

    cr.status = CutResultStatus::Ok;
    cr.planId = p.planId;
    cr.used = usedPhysical;
    cr.waste = waste;
    cr.plan = p;
    cr.result = result;
    cr.usedPieceIds = { piece.info.pieceId };
    cr.leftoverBarcode = p._segments.leftoverBarcode();   // ⭐ PATCH #5

    zInfo(QString("🎯 CUT SINGLE — OK (used=%1, waste=%2, rodId=%3)")
              .arg(cr.used)
              .arg(cr.waste)
              .arg(rod.rodId));
    return cr;
}

CutResult CutEngine::cutCombo(
    const QVector<Cutting::Piece::PieceWithMaterial>& combo,
    int remainingLength,
    const SelectedRod& rod,
    const CuttingMachine& machine,
    int currentOpId,
    int rodId,
    double kerf_mm,
    int dpLimit,
    int& planCounter)
{
    zInfo(QString("🔍 CUT COMBO — pieces=%1, remaining=%2 mm, rodId=%3")
              .arg(combo.size())
              .arg(remainingLength)
              .arg(rod.rodId));


    CutResult cr;
    cr.status = CutResultStatus::Unknown;
    cr.used = 0;
    cr.waste = 0;

   // int totalCut  = OptimizerUtils::sumLengths(combo);
   // int kerfTotal = OptimizerUtils::roundKerfLoss(combo.size(), kerf_mm);
   // int used      = totalCut + kerfTotal;
    auto info = OptimizerUtils::computePhysicalCut(combo, kerf_mm, remainingLength);

    double usedNoKerf    = info.totalCut;   // darabhossz
    double usedPhysical  = info.used;       // darabhossz + kerf

    // 1) DP-limit ellenőrzés — kerf nélkül
    if (usedNoKerf > dpLimit) {
        zInfo(QString("✖ CUT COMBO — DP túllépés (usedNoKerf=%1 > dpLimit=%2)")
                  .arg(usedNoKerf)
                  .arg(dpLimit));
        cr.status = CutResultStatus::Overfill;
        return cr;
    }

    // 2) Fizikai Overfill — kerf-fel
    if (usedPhysical > remainingLength) {
        zInfo(QString("✖ CUT COMBO — fizikai túlvágás (usedPhysical=%1 > remaining=%2)")
                  .arg(usedPhysical)
                  .arg(remainingLength));
        cr.status = CutResultStatus::Overfill;
        return cr;
    }

    double waste = remainingLength - usedPhysical;


    //int waste = remainingLength - used;

    Cutting::Plan::CutPlan p;
    p.planNumber = planCounter++;
    p.piecesWithMaterial = combo;
    // PATCH 13 — combo esetén toldás metaadat meghatározása
    // Ha bármely darab toldat, akkor a plan toldat.
    // Ha bármely darab fődarab, akkor a plan fődarab.
    // Ha vegyes, akkor toldat (a szerelés így is tudja kezelni).
    p.toldasRole = "";
    for (const auto& pc : combo) {
        if (pc.info.toldasRole == "TOLDAS_MAIN") {
            p.toldasRole = "TOLDAS_MAIN";
            break;
        }
        if (pc.info.toldasRole == "TOLDAS_TOLDAT") {
            p.toldasRole = "TOLDAS_TOLDAT";
        }
    }

    p.materialId = rod.materialId;
    p.rodId = rod.rodId;
    p.source = rod.isReusable ? Cutting::Plan::Source::Reusable : Cutting::Plan::Source::Stock;
    p.planId = QUuid::createUuid();
    p.status = Cutting::Plan::Status::NotStarted;
    p.machineId = machine.id;
    p.machineName = machine.name;
    p.machineKerf = kerf_mm;

    //int physicalLength = remainingLength;
    int physicalLength = (rod.origin == RodOrigin::Continuation) ? dpLimit : remainingLength;
    p._segments.generateSegments(combo, kerf_mm, physicalLength);
    p._segments.SetTotalLength_mm(physicalLength);

    p.sourceBarcode = rod.barcode;
    p.optimizationId = currentOpId;

    if (rod._parent.has_value()) {
        p.setParent(*rod._parent);
    } else {
        p.setParent(Cutting::Plan::ParentInfo{ rod.barcode, std::nullopt});
    }


    Cutting::Result::ResultModel result;
    result.cutPlanId = p.planId;
    result.materialId = rod.materialId;
    result.length = remainingLength;      // PATCH 1-gyel összhangban
    result.cuts = combo;
    result.waste = waste;
    result.source = rod.isReusable
                        ? Cutting::Result::ResultSource::FromReusable
                        : Cutting::Result::ResultSource::FromStock;
    result.optimizationId = rod.isReusable ? std::nullopt : std::make_optional(currentOpId);
    result.reusableBarcode = p._segments.leftoverBarcode();
    result.isFinalWaste = (waste <= 0);
    result._parent = p.parent();           // ParentInfo öröklése
    result.sourceBarcode = p.sourceBarcode;

    cr.status = CutResultStatus::Ok;
    cr.planId = p.planId;
    cr.used = usedPhysical;
    cr.waste = waste;
    cr.plan = p;
    cr.result = result;
    cr.leftoverBarcode = p._segments.leftoverBarcode();   // ⭐ PATCH #5

    // több darab → több pieceId
    for (auto& pc : combo)
        cr.usedPieceIds.append(pc.info.pieceId);

    zInfo(QString("🎯 CUT COMBO — OK (pieces=%1, used=%2, waste=%3, rodId=%4)")
              .arg(combo.size())
              .arg(cr.used)
              .arg(cr.waste)
              .arg(rod.rodId));
    return cr;
}


//QVector<Cutting::Piece::PieceWithMaterial> &groupVec);