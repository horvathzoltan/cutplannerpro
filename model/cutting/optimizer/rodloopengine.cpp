#include "rodloopengine.h"
#include "common/eventlogger.h"
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

    const MaterialMaster* mat = MaterialRegistry::instance().findById(rod.materialId);
    MaterialScoringParams sp = mat ? mat->scoringParams()
                                   : MaterialScoringParams::getDefault();

    FitEngine::FitResult fr =
        FitEngine::findBestFit(groupVec, dpLimit, kerf_mm, sp);

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
        zInfo("   ✖ Nincs több vágható darab — rúd lezárása előtt FAILED‑check");

        // ❗ Nem vágható darab felismerése
        if (!groupVec.isEmpty()) {
            auto &p = groupVec.first();

            // Fizikai szükséglet (egyszerűsített modell: darabhossz + kerf)
            int needed = p.info.length_mm + static_cast<int>(kerf_mm);

            if (needed > remainingLength) {
                int needed = p.info.length_mm + static_cast<int>(kerf_mm);
                int stockLength = rod.length;   // teljes rúd hossza
                // 1) Valódi fizikai lehetetlenség: soha nem fér fel
                if (needed > stockLength) {

                    p.failed = true;

                    DiscardedPiece dp;
                    dp.requestId  = p.info.requestId;
                    dp.materialId = p.materialId;
                    dp.machineId  = machine.id;
                    dp.failReason = QString("Nem vágható: darab hosszabb mint a teljes rúd (needed=%1, stock=%2)")
                                        .arg(needed)
                                        .arg(stockLength);
                    dp.pieceId    = p.info.pieceId;

                    model.addDiscardedPiece(dp);

                    zWarning("❌ FAILED PIECE — " + dp.failReason);

                    auto* mat = MaterialRegistry::instance().findById(p.materialId);
                    QString matName = mat ? mat->toDisplay() : "?";

                    zEvent(QString("❌ FAILED PIECE — %1 mm darab nem vágható a %2 mm rúdra "
                                   "(needed=%3, stock=%4, material=%5)")
                               .arg(p.info.length_mm)
                               .arg(stockLength)
                               .arg(needed)
                               .arg(stockLength)
                               .arg(matName));

                    groupVec.removeFirst();
                    return RodStepResult::StartNewRod;
                }

                // bool physFail = (needed > remainingLength);
                // bool dpFail   = (p.info.length_mm > dpLimit);

                // const MaterialMaster* mat2 = MaterialRegistry::instance().findById(p.materialId);
                // int stockLen = mat2 ? mat2->stockLength_mm : INT_MAX;
                // bool fitsStock = (p.info.length_mm + static_cast<int>(kerf_mm)) <= stockLen;

                // QStringList reasons;

                // if (dpFail) {
                //     reasons << QString("DP-limit túllépés: darab=%1 mm, dpLimit=%2 mm")
                //                    .arg(p.info.length_mm)
                //                    .arg(dpLimit);
                // }

                // if (physFail) {
                //     reasons << QString("Fizikai túlvágás: darab=%1 mm + kerf=%2 mm > remaining=%3 mm")
                //                    .arg(p.info.length_mm)
                //                    .arg(static_cast<int>(kerf_mm))
                //                    .arg(remainingLength);
                // }

                // if (reasons.isEmpty()) {
                //     reasons << "Ismeretlen okból nem vágható ezen a rúdon";
                // }

                // QString reason = reasons.join(" + ");

                // // ❗❗❗ STOCK-FALLBACK — NEM FAILED, csak új rúd kell
                // if (fitsStock) {

                //     reason += " (A darab STOCK rúdra vágható.)";

                //     // NEM jelöljük failed-nek
                //     // NEM mentjük DiscardedPiece-be
                //     // NEM távolítjuk el a groupVec-ből

                //     zEvent("⏩ Átmozgatva új rúdra: " + reason);

                //     // DP-limit és remainingLength reset az új rúdhoz
                //     dpLimit = stockLen;
                //     remainingLength = stockLen;

                //     // Új rúd indítása → a darab a következő rúdra kerül
                //     return RodStepResult::StartNewRod;
                // }

                // // ❌ VALÓDI FAILED — stock rúdra sem fér fel
                // reason += " (A darab STOCK rúdra sem vágható.)";

                // p.failed     = true;
                // p.failReason = reason;
                //                // + QString(" [rodLength=%1, remaining=%2, stockLen=%3]")
                //                //       .arg(rod.length)
                //                //       .arg(remainingLength)
                //                //       .arg(stockLen);

                // DiscardedPiece dp;
                // dp.requestId  = p.info.requestId;
                // dp.materialId = p.materialId;
                // dp.machineId  = machine.id;
                // dp.failReason = p.failReason;
                // dp.pieceId    = p.info.pieceId;

                // model.addDiscardedPiece(dp);

                // zEvent("❌ FAILED PIECE — " + p.failReason);

                // // Valódi FAILED → töröljük a pendingből
                // groupVec.removeFirst();

                return RodStepResult::StartNewRod;
            }

        }


        // Ha nem minősült FAILED‑nek, marad az eddigi viselkedés:
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
        zInfo("   ⚠️ Overfill — single‑piece fallback vizsgálata");
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

    if (remainingLength < sp.scrap_mm) {
        zInfo("⛔ ROD-STEP — Rúd lezárva — leftover köszöbérték alatti tartomány");
        return RodStepResult::StopRod;
    }

    if (remainingLength >= sp.goodLeftOver_Min_mm &&
        remainingLength <= sp.goodLeftOver_Max_mm)
    {
        zInfo("⛔ ROD-STEP — rúd lezárva (jó leftover tartomány, fizikai hulló képződik)");
        return RodStepResult::StopRod;
    }

    if (remainingLength >= sp.scrap_mm &&
        remainingLength < sp.goodLeftOver_Min_mm) {

        auto onePieceFit =
            OptimizerUtils::findSingleBestPiece(groupVec, dpLimit, 0.0);
        if (onePieceFit.has_value()) {
            const Cutting::Piece::PieceWithMaterial& piece = *onePieceFit;

            CutResult cr3 = model.cutSingle_AndCommit(
                piece, remainingLength, dpLimit,
                rod, machine, currentOpId, rodId, kerf_mm, groupVec);

            // if (cr3.status == CutResultStatus::Overfill) {

            //     // ❗ PATCH: ha a darab fizikailag felfér stockra → új rúd kell, nem StopRod
            //     const MaterialMaster* mat = MaterialRegistry::instance().findById(piece.materialId);
            //     int stockLen = mat ? mat->stockLength_mm : INT_MAX;
            //     int needed = piece.info.length_mm + static_cast<int>(kerf_mm);

            //     if (needed <= stockLen) {
            //         zInfo("⏭ SINGLE-FALLBACK — darab felfér stockra, új rúd indítása");
            //         return RodStepResult::StartNewRod;
            //     }

            //     // valódi FAILED
            //     return RodStepResult::StopRod;
            // }

            int newRemaining = remainingLength;

            if (newRemaining < sp.scrap_mm) {
                zInfo("⏭ ROD-STEP — új rúd indítása (aktuális rúd nem vágható tovább)");
                return RodStepResult::StartNewRod;
            } else {
                zInfo("⛔ ROD-STEP — rúd lezárva (single cut, nincs további darab)");
                return RodStepResult::StopRod;
            }
        }
    }

    if (remainingLength > sp.goodLeftOver_Max_mm) {
        auto onePieceFit =
            OptimizerUtils::findSingleBestPiece(groupVec, dpLimit, 0.0);
        if (onePieceFit.has_value()) {

            SelectedRod rod2 = rod;
            // rod2.origin = RodOrigin::Continuation; // PATCH #4 — folytatólagos rúd jelzése a cut engine-nek, hogy ne számoljon front trimet

            // if (!cr.leftoverBarcode.isEmpty()) {
            //     rod2.barcode = cr.leftoverBarcode;
            //     rod2._parent = Cutting::Plan::ParentInfo{
            //         cr.result.sourceBarcode,        // eredeti fizikai forrás (stock / reusable)
            //         std::make_optional(cr.planId)   // szülő plan UUID (pl. PLAN #8)
            //     };
            // }

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

