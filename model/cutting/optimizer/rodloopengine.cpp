#include "rodloopengine.h"
#include "optimizermodel.h"
#include "fitengine.h"
#include "../../../service/cutting/optimizer/optimizerutils.h"
#include "../../../common/logger.h"
#include "cuttypes.h"

namespace Cutting {
namespace Optimizer {

// remainingLength  = fizikai maradék
// dpLimit = DP-limit (csak ennyit használhat a FitEngine)

RodStepResult RodLoopEngine::step(
    QVector<Cutting::Piece::PieceWithMaterial>& groupVec,
    int& remainingLength,
    int& dpLimit,
    const SelectedRod& rod,
    const CuttingMachine& machine,
    int currentOpId,
    int rodId,
    double kerf_mm,
    OptimizerModel& model)
{
    QVector<int> _aff_limits;
    QVector<int> _aff_results;


    model.rodLoopIteration++;
    zInfo(QString("🔍 ROD-LOOP ITERÁCIÓ #%1 — rodId=%2, pending=%3, kerf=%4")
              .arg(model.rodLoopIteration)
              .arg(rod.rodId)
              .arg(groupVec.size())
              .arg(kerf_mm));

    zInfo(QString("🟦 ROD-LOOP LIMITS — remaining=%1, dpLimit=%2")
              .arg(remainingLength)
              .arg(dpLimit));

    FitEngine::FitResult fr =
        FitEngine::findBestFit(groupVec, dpLimit, kerf_mm);

    zInfo(QString("🔎 FitEngine hívás — dpLimit=%1 mm, pending=%2")
              .arg(dpLimit)
              .arg(groupVec.size()));


    model._fitTelemetry.accumulate(fr);

    zInfo(QString("   • FitEngine eredmény — strategy=%1, picks=%2, waste=%3, rodId=%4")
              .arg(fr.strategyString())
              .arg(fr.pieceCount)
              .arg(fr.waste)
              .arg(rod.rodId));

    _aff_limits.append(dpLimit);
    _aff_results.append(fr.pieceCount);

    if (fr.combo.isEmpty()){
        zInfo("   ✖ Nincs több vágható darab — rúd lezárása");
        return RodStepResult::StopRod;
    }

    zInfo(QString("   ✔ Combo sikeres — %1 darab kiválasztva, used=%2 mm")
              .arg(fr.pieceCount)
              .arg(fr.used));

    const QVector<Cutting::Piece::PieceWithMaterial>& combo = fr.combo;

    CutResult cr = model.cutCombo_AndCommit(
        combo, remainingLength, dpLimit,
        rod, machine, currentOpId, rodId, kerf_mm, groupVec);


    if (cr.status == CutResultStatus::Overfill)
    {
        zInfo("   ⚠ Overfill — single‑piece fallback vizsgálata");
        std::optional<Cutting::Piece::PieceWithMaterial> single =
            OptimizerUtils::findSingleBestPiece(groupVec, dpLimit, 0.0);
        if (single.has_value()) {
            CutResult cr2 = model.cutSingle_AndCommit(
                *single, remainingLength, dpLimit,
                rod, machine, currentOpId, rodId, kerf_mm, groupVec);

            if (cr2.status == CutResultStatus::Overfill)
            {
                remainingLength  = 0;
                dpLimit = 0;
                return RodStepResult::StopRod;
            }

            remainingLength  = 0;
            dpLimit = 0;
            return RodStepResult::StopRod;
        }

        remainingLength  = 0;
        dpLimit = 0;

        zInfo("⛔ ROD-STEP — rúd lezárva (túlvágás elleni védelem aktiválva)");
        return RodStepResult::StopRod;
    }

    if (remainingLength < OptimizerConstants::SELEJT_THRESHOLD) {
        zInfo("⛔ ROD-STEP — Rúd lezárva — leftover köszöbérték alatti tartomány");
        return RodStepResult::StopRod;
    }

    if (remainingLength >= OptimizerConstants::GOOD_LEFTOVER_MIN &&
        remainingLength <= OptimizerConstants::GOOD_LEFTOVER_MAX) {
        zInfo("⛔ ROD-STEP — rúd lezárva (jó leftover tartomány, fizikai hulló képződik)");
        return RodStepResult::StopRod;
    }

    if (remainingLength >= OptimizerConstants::SELEJT_THRESHOLD &&
        remainingLength < OptimizerConstants::GOOD_LEFTOVER_MIN) {

        auto onePieceFit =
            OptimizerUtils::findSingleBestPiece(groupVec, dpLimit, 0.0);
        if (onePieceFit.has_value()) {
            const Cutting::Piece::PieceWithMaterial& piece = *onePieceFit;

            CutResult cr3 = model.cutSingle_AndCommit(
                piece, remainingLength, dpLimit,
                rod, machine, currentOpId, rodId, kerf_mm, groupVec);

            if (cr3.status == CutResultStatus::Overfill) {
                return RodStepResult::StopRod;
            }

            int newRemaining = remainingLength;

            if (newRemaining < OptimizerConstants::SELEJT_THRESHOLD) {
                zInfo("⏭ ROD-STEP — új rúd indítása (aktuális rúd nem vágható tovább)");
                return RodStepResult::StartNewRod;
            } else {
                zInfo("⛔ ROD-STEP — rúd lezárva (single cut, nincs további darab)");
                return RodStepResult::StopRod;
            }
        }
    }

    if (remainingLength > OptimizerConstants::GOOD_LEFTOVER_MAX) {
        auto onePieceFit =
            OptimizerUtils::findSingleBestPiece(groupVec, dpLimit, 0.0);
        if (onePieceFit.has_value()) {

            SelectedRod rod2 = rod;
            rod2.origin = RodOrigin::Continuation; // PATCH #4 — folytatólagos rúd jelzése a cut engine-nek, hogy ne számoljon front trimet

            if (!cr.leftoverBarcode.isEmpty()) {
                rod2.barcode = cr.leftoverBarcode;
                rod2._parent = Cutting::Plan::ParentInfo{                    
                    cr.result.sourceBarcode,        // eredeti fizikai forrás (stock / reusable)
                    std::make_optional(cr.planId)   // szülő plan UUID (pl. PLAN #8)
                };
            }

            CutResult cr4 = model.cutSingle_AndCommit(
                *onePieceFit, remainingLength, dpLimit,
                rod2,
                machine, currentOpId,
                rodId, kerf_mm, groupVec);

            Q_UNUSED(cr4);
            zInfo("➡ ROD-STEP — folytatás ugyanazzal a rúddal (van még vágható "
                  "darab)");
            return RodStepResult::ContinueSameRod;
        }
        return RodStepResult::StopRod;
    }

    QStringList limitsStr, resultsStr;
    for (int v : _aff_limits)  limitsStr << QString::number(v);
    for (int v : _aff_results) resultsStr << QString::number(v);

    zInfo(QString("📊 Iteráció összegzés — attempts=%1, limits=[%2], results=[%3], rodId=%4")
              .arg(_aff_limits.size())
              .arg(limitsStr.join(","))
              .arg(resultsStr.join(","))
              .arg(rod.rodId));



    return RodStepResult::StopRod;
}

} // namespace Optimizer
} // namespace Cutting

