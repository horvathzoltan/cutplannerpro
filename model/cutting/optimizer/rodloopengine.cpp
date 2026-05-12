#include "rodloopengine.h"
#include "optimizermodel.h"
#include "fitengine.h"
#include "../../../service/cutting/optimizer/optimizerutils.h"
#include "../../../common/logger.h"
#include "cuttypes.h"

namespace Cutting {
namespace Optimizer {

RodStepResult RodLoopEngine::step(
    QVector<Cutting::Piece::PieceWithMaterial>& groupVec,
    int& remainingLength,
    int& remainingLength2,
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
    //zInfo(QString("rodLoop iteration #%1").arg(model.rodLoopIteration));

    FitEngine::FitResult fr =
        FitEngine::findBestFit(groupVec, remainingLength2, kerf_mm);

    zInfo(QString("rodLoop iteration #%1 → limit=%2")
              .arg(model.rodLoopIteration)
              .arg(remainingLength2));

    // zInfo(QString("rodLoop iteration #%1 → bestFit → strategy=%2, picks=%3, waste=%4, limit=%5")
    //           .arg(model.rodLoopIteration)
    //           .arg(static_cast<int>(fr.strategy))
    //           .arg(fr.pieceCount)
    //           .arg(fr.waste)
    //           .arg(remainingLength2));

    model._fitTelemetry.accumulate(fr);

    zInfo(QString("    strategy=%1, picks=%2, waste=%3")
              .arg(fr.strategyString())
              .arg(fr.pieceCount)
              .arg(fr.waste));

    _aff_limits.append(remainingLength2);
    _aff_results.append(fr.pieceCount);

    if (fr.combo.isEmpty()){
            zInfo("    result: FAILED");
        return RodStepResult::StopRod;
    }

    zInfo("    result: SUCCESS");

    const QVector<Cutting::Piece::PieceWithMaterial>& combo = fr.combo;

    CutResult cr = model.cutCombo_AndCommit(
        combo, remainingLength, remainingLength2,
        rod, machine, currentOpId, rodId, kerf_mm, groupVec);


    if (cr.status == CutResultStatus::Overfill)
    {
        std::optional<Cutting::Piece::PieceWithMaterial> single =
            OptimizerUtils::findSingleBestPiece(groupVec, remainingLength2, kerf_mm);
        if (single.has_value()) {
            CutResult cr2 = model.cutSingle_AndCommit(
                *single, remainingLength, remainingLength2,
                rod, machine, currentOpId, rodId, kerf_mm, groupVec);

            if (cr2.status == CutResultStatus::Overfill)
            {
                remainingLength  = 0;
                remainingLength2 = 0;
                return RodStepResult::StopRod;
            }

            remainingLength  = 0;
            remainingLength2 = 0;
            return RodStepResult::StopRod;
        }

        remainingLength  = 0;
        remainingLength2 = 0;

        zInfo("rodLoop result: STOP (overfill)");
        return RodStepResult::StopRod;
    }

    if (remainingLength < OptimizerConstants::SELEJT_THRESHOLD) {
        zInfo("rodLoop result: STOP (below SELEJT_THRESHOLD)");
        return RodStepResult::StopRod;
    }

    if (remainingLength >= OptimizerConstants::GOOD_LEFTOVER_MIN &&
        remainingLength <= OptimizerConstants::GOOD_LEFTOVER_MAX) {
        zInfo("rodLoop result: STOP (good leftover range)");
        return RodStepResult::StopRod;
    }

    if (remainingLength >= OptimizerConstants::SELEJT_THRESHOLD &&
        remainingLength < OptimizerConstants::GOOD_LEFTOVER_MIN) {

        auto onePieceFit =
            OptimizerUtils::findSingleBestPiece(groupVec, remainingLength, kerf_mm);
        if (onePieceFit.has_value()) {
            const Cutting::Piece::PieceWithMaterial& piece = *onePieceFit;

            CutResult cr3 = model.cutSingle_AndCommit(
                piece, remainingLength, remainingLength2,
                rod, machine, currentOpId, rodId, kerf_mm, groupVec);

            if (cr3.status == CutResultStatus::Overfill) {
                return RodStepResult::StopRod;
            }

            int newRemaining = remainingLength;

            if (newRemaining < OptimizerConstants::SELEJT_THRESHOLD) {
                zInfo("rodLoop result: START_NEW_ROD");
                return RodStepResult::StartNewRod;
            } else {
                zInfo("rodLoop result: STOP (after single cut)");
                return RodStepResult::StopRod;
            }
        }
    }

    if (remainingLength > OptimizerConstants::GOOD_LEFTOVER_MAX) {
        auto onePieceFit =
            OptimizerUtils::findSingleBestPiece(groupVec, remainingLength, kerf_mm);
        if (onePieceFit.has_value()) {
            CutResult cr4 = model.cutSingle_AndCommit(
                *onePieceFit, remainingLength, remainingLength2,
                rod, machine, currentOpId, rodId, kerf_mm, groupVec);

            Q_UNUSED(cr4);
            zInfo("rodLoop result: CONTINUE_SAME_ROD");
            return RodStepResult::ContinueSameRod;
        }
        return RodStepResult::StopRod;
    }

    QStringList limitsStr, resultsStr;
    for (int v : _aff_limits)  limitsStr << QString::number(v);
    for (int v : _aff_results) resultsStr << QString::number(v);

    zInfo(QString("🧩 FitEngine::findBestFit attempts=%1 limits=%2 results=%3")
              .arg(_aff_limits.size())
              .arg(limitsStr.join(","))
              .arg(resultsStr.join(",")));


    return RodStepResult::StopRod;
}

} // namespace Optimizer
} // namespace Cutting


//             QVector<Cutting::Piece::PieceWithMaterial> combo =
//                 FitEngine::findBestFit(groupVec, remainingLength2, kerf_mm);

//             zInfo(QString("findBestFit: %1 darab, bestCombo size=%2")
//                       .arg(groupVec.size())
//                       .arg(combo.size()));

//             if (combo.isEmpty())
//                 break;

//             CutResult cr =
//                 cutCombo_WithLifecycle(combo, remainingLength, remainingLength2,
//                                                   rod, machine, currentOpId, rodId,
//                                                   kerf_mm, groupVec);

//              if (cr.status == CutResultStatus::Overfill)
//             {
//                 // próbáljunk single-piece fallbacket
//                 std::optional<Cutting::Piece::PieceWithMaterial> single =
//                     OptimizerUtils::findSingleBestPiece(groupVec, remainingLength2, kerf_mm);
//                 if (single.has_value()) {
//                     CutResult cr2 =
//                         cutSingle_WithLifecycle(*single, remainingLength, remainingLength2,
//                                                             rod, machine, currentOpId,
//                                                             rodId, kerf_mm, groupVec);


//                     if (cr2.status == CutResultStatus::Overfill)
//                     {
//                         remainingLength = 0;
//                         remainingLength2 = 0;
//                         break;
//                     }

//                     // fallback után az eredeti logika szerint lezárjuk a rudat
//                     remainingLength = 0;
//                     remainingLength2 = 0;
//                     break;

//                 }

//                 // ha single-piece sem megy → lezárjuk
//                 remainingLength = 0;
//                 remainingLength2 = 0;
//                 break;
//             }


//             // Selejt alá esés → lezárás
//             if (remainingLength < OptimizerConstants::SELEJT_THRESHOLD) {
//                 // if (remainingLength > 0) {
//                 //     LeftoverStockEntry entry;
//                 //     entry.materialId = rod.materialId;
//                 //     entry.availableLength_mm = remainingLength;
//                 //     entry.used = false;
//                 //     entry.barcode = QString("UU1-%1").arg(QUuid::createUuid().toString().mid(1, 6));
//                 //     _localLeftovers.append(entry);
//                 // }
//                 break;
//             } /*else {
//                 remainingLength  -= cr.used;
//                 remainingLength2 -= cr.used;
//             }*/
//             // különben nem csinálunk semmit: applyLifecycle már csökkentette a remaininget

//             // Jó leftover tartomány (500–800) → lezárás
//             if (remainingLength >= OptimizerConstants::GOOD_LEFTOVER_MIN &&
//                 remainingLength <= OptimizerConstants::GOOD_LEFTOVER_MAX) {
//                 break;
//             }

//             // Köztes tartomány (300–500) → próbáljunk megszabadulni tőle
//             if (remainingLength >= OptimizerConstants::SELEJT_THRESHOLD &&
//                 remainingLength < OptimizerConstants::GOOD_LEFTOVER_MIN) {

//                 auto onePieceFit = OptimizerUtils::findSingleBestPiece(groupVec, remainingLength, kerf_mm);
//                 if (onePieceFit.has_value()) {
//                     const Piece::PieceWithMaterial &piece = *onePieceFit;

//                     CutResult cr3 =
//                         cutSingle_WithLifecycle(piece, remainingLength, remainingLength2,
//                                                             rod, machine,
//                                                             currentOpId, rodId, kerf_mm, groupVec);

//                     if (cr3.status == CutResultStatus::Overfill) {
//                         break;
//                     }

//                     int newRemaining = remainingLength;


//                     // applyLifecycle már csökkentette a remaininget
//                     //int newRemaining = remainingLength - cr3.used;
//                     // remainingLength  -= cr3.used;
//                     // remainingLength2 -= cr3.used;


//                     if (newRemaining < OptimizerConstants::SELEJT_THRESHOLD) {
//                         // teljesen elfogyott → új rúd
//                         continue;
//                     } else {
//                         // jó leftover → lezárjuk
//                         break;
//                     }
//                 }
//             }

//             // Túl nagy leftover (> 800) → próbáljunk még egy darabot
//                 if (remainingLength > OptimizerConstants::GOOD_LEFTOVER_MAX) {
//                     auto onePieceFit = OptimizerUtils::findSingleBestPiece(groupVec, remainingLength, kerf_mm);
//                     if (onePieceFit.has_value()) {
//                         CutResult cr4 =
//                             cutSingle_WithLifecycle(*onePieceFit, remainingLength, remainingLength2,
//                                                                 rod, machine,
//                                                                 currentOpId, rodId, kerf_mm, groupVec);

//                        // remainingLength  -= cr4.used;
//                         //remainingLength2 -= cr4.used;
// // applyLifecycle már csökkentette a remaininget

//                         continue; // folytatjuk a rod-loopot
//                     }
//                     break; // ha nincs, lezárjuk
//                 }

//             break; // nincs több értelmes darab → lezárjuk
