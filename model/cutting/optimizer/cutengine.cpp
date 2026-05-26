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

    int used = piece.info.length_mm + OptimizerUtils::roundKerfLoss(1, kerf_mm);

    if (used > remainingLength) {
        zInfo(QString("✖ CUT SINGLE — túlvágás történt (used=%1 > remaining=%2)")
                  .arg(used)
                  .arg(remainingLength));
        cr.status = CutResultStatus::Overfill;
        return cr;
    }

    int waste = remainingLength - used;

    Cutting::Plan::CutPlan p;
    p.planNumber = planCounter++;
    p.piecesWithMaterial = { piece };
    p.materialId = rod.materialId;
    p.rodId = rod.rodId;
    p.source = rod.isReusable ? Cutting::Plan::Source::Reusable : Cutting::Plan::Source::Stock;
    p.planId = QUuid::createUuid();
    p.status = Cutting::Plan::Status::NotStarted;
    p.machineId = machine.id;
    p.machineName = machine.name;
    p.machineKerf = kerf_mm;

    int physicalLength = (rod.origin == RodOrigin::Continuation) ? dpLimit : remainingLength;
    p._segments.generateSegments({piece}, kerf_mm, physicalLength);
    p._segments.SetTotalLength_mm(physicalLength);

    p.sourceBarcode = rod.barcode;
    p.optimizationId = currentOpId;

    if (rod.isReusable) {
        p._parent = rod._parent;   // leftover → örökli a szülőt
    } else {
        p._parent = Cutting::Plan::ParentInfo{ rod.barcode, std::nullopt };
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
    result._parent = p._parent; // ParentInfo öröklése
    result.sourceBarcode = p.sourceBarcode;

    cr.status = CutResultStatus::Ok;
    cr.planId = p.planId;
    cr.used = used;
    cr.waste = waste;
    cr.plan = p;
    cr.result = result;
    cr.usedPieceIds = { piece.info.pieceId };

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

    int totalCut  = OptimizerUtils::sumLengths(combo);
    int kerfTotal = OptimizerUtils::roundKerfLoss(combo.size(), kerf_mm);
    int used      = totalCut + kerfTotal;

    if (used > remainingLength) {
        zInfo(QString("✖ CUT COMBO — túlvágás történt (used=%1 > remaining=%2)")
                  .arg(used)
                  .arg(remainingLength));
        cr.status = CutResultStatus::Overfill;
        return cr;
    }

    int waste = remainingLength - used;

    Cutting::Plan::CutPlan p;
    p.planNumber = planCounter++;
    p.piecesWithMaterial = combo;
    p.materialId = rod.materialId;
    p.rodId = rod.rodId;
    p.source = rod.isReusable ? Cutting::Plan::Source::Reusable : Cutting::Plan::Source::Stock;
    p.planId = QUuid::createUuid();
    p.status = Cutting::Plan::Status::NotStarted;
    p.machineId = machine.id;
    p.machineName = machine.name;
    p.machineKerf = kerf_mm;

    int physicalLength = (rod.origin == RodOrigin::Continuation) ? dpLimit : remainingLength;
    p._segments.generateSegments(combo, kerf_mm, physicalLength);
    p._segments.SetTotalLength_mm(physicalLength);

    p.sourceBarcode = rod.barcode;
    p.optimizationId = currentOpId;

    if (rod.isReusable) {
        p._parent = rod._parent;   // leftover → örökli a szülőt
    } else {
        p._parent = Cutting::Plan::ParentInfo{ rod.barcode, std::nullopt };
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
    result._parent = p._parent;           // ParentInfo öröklése
    result.sourceBarcode = p.sourceBarcode;

    cr.status = CutResultStatus::Ok;
    cr.planId = p.planId;
    cr.used = used;
    cr.waste = waste;
    cr.plan = p;
    cr.result = result;

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