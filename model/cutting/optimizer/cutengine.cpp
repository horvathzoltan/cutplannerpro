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
    p.kerfTotal = OptimizerUtils::roundKerfLoss(1, kerf_mm);
    p.waste = waste;
    p.materialId = rod.materialId;
    p.rodId = rod.rodId;
    p.source = rod.isReusable ? Cutting::Plan::Source::Reusable : Cutting::Plan::Source::Stock;
    p.planId = QUuid::createUuid();
    p.status = Cutting::Plan::Status::NotStarted;
    p.totalLength = rod.length;
    p.machineId = machine.id;
    p.machineName = machine.name;
    p.kerfUsed_mm = kerf_mm;
    p.generateSegments(kerf_mm, rod.length);
    p.sourceBarcode = rod.barcode;
    p.optimizationId = currentOpId;
    p.parentBarcode = rod.barcode;
    p.parentPlanId = std::nullopt;
    p.leftoverBarcode = p.getWasteBarcode();

    Cutting::Result::ResultModel result;
    result.cutPlanId = p.planId;
    result.materialId = rod.materialId;
    result.length = rod.length;
    result.cuts = { piece };
    result.waste = waste;
    result.source = rod.isReusable
                        ? Cutting::Result::ResultSource::FromReusable
                        : Cutting::Result::ResultSource::FromStock;
    result.optimizationId = rod.isReusable ? std::nullopt : std::make_optional(currentOpId);
    result.reusableBarcode = p.leftoverBarcode;
    result.isFinalWaste = (waste <= 0);
    result.parentBarcode = p.parentBarcode;
    result.sourceBarcode = rod.barcode;

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
    p.kerfTotal = kerfTotal;
    p.waste = waste;
    p.materialId = rod.materialId;
    p.rodId = rod.rodId;
    p.source = rod.isReusable ? Cutting::Plan::Source::Reusable : Cutting::Plan::Source::Stock;
    p.planId = QUuid::createUuid();
    p.status = Cutting::Plan::Status::NotStarted;
    p.totalLength = rod.length;
    p.machineId = machine.id;
    p.machineName = machine.name;
    p.kerfUsed_mm = kerf_mm;
    p.generateSegments(kerf_mm, rod.length);
    p.sourceBarcode = rod.barcode;
    p.optimizationId = currentOpId;
    p.parentBarcode = rod.barcode;
    p.parentPlanId = std::nullopt;
    p.leftoverBarcode = p.getWasteBarcode();

    Cutting::Result::ResultModel result;
    result.cutPlanId = p.planId;
    result.materialId = rod.materialId;
    result.length = rod.length;
    result.cuts = combo;
    result.waste = waste;
    result.source = rod.isReusable
                        ? Cutting::Result::ResultSource::FromReusable
                        : Cutting::Result::ResultSource::FromStock;
    result.optimizationId = rod.isReusable ? std::nullopt : std::make_optional(currentOpId);
    result.reusableBarcode = p.leftoverBarcode;
    result.isFinalWaste = (waste <= 0);
    result.parentBarcode = p.parentBarcode;
    result.sourceBarcode = rod.barcode;

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