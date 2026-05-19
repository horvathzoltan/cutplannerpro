#include <algorithm>
#include <QSet>
#include <QCoreApplication>
#include <QThread>
#include <QDebug>
#include <QElapsedTimer>

#include <model/registries/storageregistry.h>

#include "cutengine.h"
#include "cuttypes.h"
#include "optimizermodel.h"
#include "reusablefitengine.h"
#include "stockfitengine.h"
#include "../../../common/eventlogger.h"
#include "../../../common/logger.h"
#include "materials/utils/material_group_utils.h"
#include "../../machine/machineutils.h"
#include "../../../service/cutting/optimizer/optimizerutils.h"
//#include "service/cutting/segment/segmentutils.h"
//#include "model/profilestock.h"
//#include <numeric>
#include "../../../common/identifierutils.h"
#include "../../../common/settingsmanager.h"
#include "rodloopengine.h"


namespace Cutting {
namespace Optimizer {

OptimizerModel::OptimizerModel(QObject *parent) : QObject(parent) {}

QVector<Cutting::Plan::CutPlan>& OptimizerModel::getResult_PlansRef() {
    return _result_plans;
}

QVector<Cutting::Result::ResultModel> OptimizerModel::getResults_Leftovers() const {
    return _planned_leftovers;
}

void OptimizerModel::optimize(TargetHeuristic heuristic) {
    int currentOpId = nextOptimizationId++;


    zEvent(QString("⏳ Optimize(%2) run started (heuristic=%1)")
               .arg(heuristic == TargetHeuristic::ByCount ? "ByCount" : "ByTotalLength")
               .arg(currentOpId));

    QElapsedTimer timer;
    timer.start();

    _fitTelemetry = {};
    rodLoopIteration = 0;
    rodCounter = 0;

    _result_plans.clear();
    _planned_leftovers.clear();
    _usedLeftoverEntryIds.clear();
    _localLeftovers.clear();
    leftoverRodMap.clear();

    // plusz a lokális pool, ha van
    // 🔹 Lokális leftover snapshot
    //QVector<LeftoverStockEntry> leftovers = _inventorySnapshot.reusableInventory;
    //leftovers += _localLeftovers;
    //_localLeftovers.clear();
    //_localLeftovers.clear(); // tisztán indulunk

    // 🔹 Csak globális snapshot készül – lokális pool külön marad
    QVector<LeftoverStockEntry> globalSnapshot = _inventorySnapshot.reusableInventory;

    zInfo("📦 INVENTORY SNAPSHOT — újrahasznosítható hullók");
    for (const auto& e : _inventorySnapshot.reusableInventory) {
        const MaterialMaster* mat = MaterialRegistry::instance().findById(e.materialId);
        const StorageEntry* st = StorageRegistry::instance().findById(e.storageId);

        zInfo(QString("   • Hulló: barcode=%1, hossz=%2 mm, anyag=%3, tároló=%4, forrás=%5")
                  .arg(e.barcode)
                  .arg(e.availableLength_mm)
                  .arg(mat ? mat->toDisplay() : e.materialId.toString())
                  .arg(st ? st->name : "ismeretlen")
                  .arg(e.sourceAsString()));

    }
    zInfo("📘 SNAPSHOT — vége");


    QHash<QUuid, QVector<Cutting::Piece::PieceWithMaterial>> piecesByMaterial;

    for (const Cutting::Plan::Request &req : _requests) {

        int leftRemaining  = req.leftCount;
        int rightRemaining = req.rightCount;

        for (int i = 0; i < req.quantity; ++i) {

            Cutting::Piece::PieceInfo info;
            info.length_mm = req.requiredLength;
            info.requestId = req.requestId;
            info.isCompleted = false;

            // ⭐ DARAB-SZÁMOZÁS
            if (req.quantity > 1) {
                info.externalReference = QString("%1 %2/%3")
                .arg(req.externalReference)
                    .arg(i + 1)
                    .arg(req.quantity);
            } else {
                info.externalReference = req.externalReference;
            }

            // ⭐ HandlerSide kiosztása
            HandlerSide side = HandlerSide::None;
            if (leftRemaining > 0) {
                side = HandlerSide::Left;
                leftRemaining--;
            } else if (rightRemaining > 0) {
                side = HandlerSide::Right;
                rightRemaining--;
            }

            // ⭐ PieceWithMaterial létrehozása
            Cutting::Piece::PieceWithMaterial pwm(info, req.materialId);

            // ⭐ Oldal beállítása
            pwm.side = side;

            // ⭐ Altípus átadása
            pwm.subtype = req.subtype;

            piecesByMaterial[req.materialId].append(pwm);
        }
    }

    auto anyPending = [&]() {
        for (auto it = piecesByMaterial.begin(); it != piecesByMaterial.end(); ++it)
            if (!it.value().isEmpty()) return true;
        return false;
    };

    int rodId = 0;

    static int counter = 0;

    zInfo("🔍 OPTIMALIZÁCIÓ INDÍTÁSA — pending darabok keresése");
    // 2. Optimalizációs ciklus
    while (anyPending()) {
        zInfo(QString("🔎 OPTIMIZER LOOP #%1 — pending darabok vizsgálata").arg(counter));
        // PATCH #15 — pending statisztika

        int totalPending = 0;
        int totalLength = 0;
        QUuid maxMat;
        int maxCount = 0;

        for (auto it = piecesByMaterial.begin(); it != piecesByMaterial.end(); ++it) {
            int count = it.value().size();
            if (count == 0) continue;

            int sumLen = OptimizerUtils::sumLengths(it.value());
            totalPending += count;
            totalLength += sumLen;

            if (count > maxCount) {
                maxCount = count;
                maxMat = it.key();
            }

            const MaterialMaster* mm = MaterialRegistry::instance().findById(it.key());
            zInfo(QString("   • Anyag=%1 → %2 db, összhossz=%3 mm")
                      .arg(mm ? mm->toDisplay() : it.key().toString())
                      .arg(count)
                      .arg(sumLen));
        }

        const MaterialMaster* maxMatPtr = MaterialRegistry::instance().findById(maxMat);
        zInfo(QString("📊 Pending összegzés — összes=%1 db, összhossz=%2 mm, legnagyobb anyagcsoport=%3 (%4 db)")
                  .arg(totalPending)
                  .arg(totalLength)
                  .arg(maxMatPtr ? maxMatPtr->toDisplay() : maxMat.toString())
                  .arg(maxCount));


        if (counter % 50 == 0) {            
            QCoreApplication::processEvents(QEventLoop::AllEvents, 20);
            //QThread::msleep(1); // 10 ms szünet minden 100 iterációban, hogy ne blokkoljuk a UI-t
        }
        counter++;
        // 2/a. Anyagcsoport kiválasztása
        QUuid targetMaterialId;
        int bestMetric = -1;
        for (auto it = piecesByMaterial.begin(); it != piecesByMaterial.end(); ++it) {
            int metric = (heuristic == TargetHeuristic::ByCount)
            ? it.value().size()
            : OptimizerUtils::sumLengths(it.value());
            if (metric > bestMetric) {
                bestMetric = metric;
                targetMaterialId = it.key();
            }
        }

        auto &groupVec = piecesByMaterial[targetMaterialId];
        if (groupVec.isEmpty()) {
            zInfo("✖ Nincs több darab ehhez az anyagcsoporthoz — továbblépés");
            continue;
        }
        zInfo(QString("✔ Aktuális anyagcsoport pending: %1 db").arg(groupVec.size()));


        // 2/b. Gép kiválasztása

        // PATCH #16 — MachineSelect logolás
        const MaterialMaster* matForMachine = MaterialRegistry::instance().findById(targetMaterialId);
        zInfo(QString("🔍 GÉP KERESÉSE — anyag=%1")
                  .arg(matForMachine ? matForMachine->toDisplay() : targetMaterialId.toString()));


        auto machineOpt = MachineUtils::pickMachineForMaterial(targetMaterialId);
        if (!machineOpt) {
                zInfo("✖ Nincs kompatibilis gép — pending darab eldobása");
                groupVec.removeFirst();
                continue;
        }
        const CuttingMachine machine = *machineOpt;


        zInfo(QString("✔ Kiválasztott gép: %1 (kerf=%2 mm, maxLen=%3)")
                  .arg(machine.name)
                  .arg(machine.kerf_mm)
                  .arg(machine.stellerMaxLength_mm.has_value()
                           ? QString::number(*machine.stellerMaxLength_mm)
                           : "n/a"));

        const double kerf_mm = machine.kerf_mm;
        zInfo(QString("   • Gép kerf érték: %1 mm").arg(kerf_mm));

        //2/c. Rúd kiválasztása

        const MaterialMaster* mat1 = MaterialRegistry::instance().findById(targetMaterialId);

        zInfo(QString("🔍 ANYAGCSOPORT KERESÉSE — jelölt: %1 (%2 db pending)")
                  .arg(mat1 ? mat1->toDisplay() : targetMaterialId.toString())
                  .arg(groupVec.size()));

        QSet<QUuid> groupedMaterialIds = GroupUtils::groupMembers(targetMaterialId);

        QStringList groupIdStrings;
        for (const QUuid &id : groupedMaterialIds){
            const MaterialMaster* mat1 = MaterialRegistry::instance().findById(targetMaterialId);

            if(mat1){
                groupIdStrings << mat1->toDisplay();
            } else{
                groupIdStrings << id.toString();
            }

        }

        zInfo(QString("✔ Kiválasztott anyagcsoport: %1").arg(groupIdStrings.join(", ")));
        SelectedRod rod;

        // fizikai rúd hossza (vágás után csökken)
        int remainingLength = 0;
        //  DP limit (ami a FitEngine számára engedélyezett maximum)
        int dpLimit = 0;
        bool rodSelected = false;

        // Összefésült nézet készítése
        auto merged = globalSnapshot;
        merged += _localLeftovers;

        // ♻️ Először próbáljunk reusable-t
        // REUSEABLE ÁG

        zInfo("🔍 RÚD KERESÉSE — először hulló, majd stock vizsgálata");


        std::optional<ReusableCandidate> candidate =
            ReusableFitEngine::findBestReusableFit(merged,globalSnapshot.size(),
                groupVec,targetMaterialId, kerf_mm, _usedLeftoverEntryIds, *this);
        if (candidate.has_value()
            && (
                candidate->stock.source == Cutting::Result::LeftoverSource::Optimization
                ||  candidate->stock.source == Cutting::Result::LeftoverSource::Manual
                )
            )

        {
            const auto &best = *candidate;

            // ⛔ AZONNALI tiltás, még mielőtt bármi mást csinálnánk
            _usedLeftoverEntryIds.insert(best.stock.entryId);

            rod.materialId = best.stock.materialId;
            rod.length = best.stock.availableLength_mm;
            rod.isReusable = true;
            rod.barcode = best.stock.barcode;
            rod.entryId = best.stock.entryId;


            // 🔍 RodId hozzárendelés a map alapján, fallback + azonnali regisztráció
            if (leftoverRodMap.contains(best.stock.entryId)) {
                rod.rodId = leftoverRodMap.value(best.stock.entryId);
                // zInfo(QString("MAP-LOOKUP: %1 → %2")
                //           .arg(best.stock.entryId.toString())
                //           .arg(rod.rodId));
                zInfo(QString("REUSE ROD FROM LEFTOVER: rodId=%2, stock=%3")
                          .arg(rod.rodId)
                          .arg(best.stock.barcode));
            } else {
                rod.rodId = IdentifierUtils::makeRodId(++rodCounter);
                const MaterialMaster* m = MaterialRegistry::instance().findById(rod.materialId);
                zInfo(QString("🆔 NEW ROD ID: %1 (source=stock, material=%2, length=%3)")
                          .arg(rod.rodId)
                          .arg(m?m->toDisplay():rod.materialId.toString())
                          .arg(rod.length));

                zWarning(QString("NEW ROD FOR LEFTOVER: stock=%1, rodCounter=%2, rodId=%3")
                             .arg(best.stock.barcode)
                             .arg(rodCounter)
                             .arg(rod.rodId));

                // zWarning(QString("⚠️ Missing rodId mapping for leftover %1, new rodId=%2")
                //           .arg(best.stock.entryId.toString())
                //           .arg(rod.rodId));
                // 🔑 Azonnal regisztráljuk, hogy a lánc ne szakadjon meg
                leftoverRodMap.insert(best.stock.entryId, rod.rodId);
                zInfo(QString("MAP-INSERT (on select): %1 → %2")
                          .arg(best.stock.barcode)
                          .arg(rod.rodId));
            }

            // ⛔ Azonnali tiltás, hogy ne válasszuk újra ugyanebben az iterációban
            //_usedLeftoverEntryIds.insert(best.stock.entryId);

            //zEvent(QString("♻️ Forrás leftover tiltva: %1").arg(best.stock.entryId.toString()));

            // leftovers.removeAt(best.indexInInventory);
            // consumeLeftover(best.stock);
            //  ⬇️ Fogyasztás a forrás alapján, nem indexből
            if (best.source == ReusableCandidate::Source::GlobalSnapshot) {
                // Globálisból törlünk barcode alapján
                auto &global =
                    globalSnapshot; //_inventorySnapshot.reusableInventory;
                global.erase(std::remove_if(global.begin(), global.end(),
                                            [&](const LeftoverStockEntry &e) {
                                                return e.entryId ==
                                        best.stock.entryId;
                             }),
                             global.end());
            } else {
                // Lokális poolból törlünk barcode alapján
                _localLeftovers.erase(
                    std::remove_if(_localLeftovers.begin(), _localLeftovers.end(),
                                   [&](const LeftoverStockEntry &e) {
                                       return e.entryId == best.stock.entryId;
                    }),
                    _localLeftovers.end());
            }

            remainingLength = rod.length;

            // [ tiszta vágott él ]
            // [ -------- hasznos hossz -------- ]
            // [ 70 mm min. hulló ]
            // [ 15 mm gyári vég ]  ← ebből vesszük el a front trimet

            dpLimit = rod.length
                               - OptimizerConstants::END_TRIM_MM        // gyári vég
                               - OptimizerConstants::MINIMUM_HULLO_MM   // mechanikai minimum
                               - OptimizerUtils::roundKerfLoss(1, kerf_mm);

            rodSelected = true;
            // reusable ág végén
            zInfo(QString("SELECTED REUSABLE ROD: rodId=%1, barcode=%2, length=%3")
                      .arg(rod.rodId)
                      .arg(rod.barcode)
                      .arg(rod.length));
        }
        else
        {
            //STOCK ÁG

            zInfo("♻️ No reusable leftover fits — falling back to stock.");

            QSet<QUuid> groupIds = GroupUtils::groupMembers(targetMaterialId);

            auto stockRod = StockFitEngine::pickStockRod(
                _inventorySnapshot.profileInventory,
                groupIds,
                rodCounter);

            if (stockRod.has_value()) {
                rod = *stockRod;

                remainingLength = rod.length;
                dpLimit = rod.length
                                   - OptimizerConstants::END_TRIM_MM        // front trim
                                   - OptimizerConstants::END_TRIM_MM        // gyári vég
                                   - OptimizerConstants::MINIMUM_HULLO_MM   // mechanikai minimum
                                   - OptimizerUtils::roundKerfLoss(1, kerf_mm);

                rodSelected = true;

                zInfo(QString("🟦 ROD SELECTED — rodId=%1, barcode=%2, length=%3, reusable=%4, entryId=%5")
                          .arg(rod.rodId)
                          .arg(rod.barcode)
                          .arg(rod.length)
                          .arg(rod.isReusable)
                          .arg(rod.entryId.has_value() ? rod.entryId->toString() : "—"));

                zInfo(QString("🟦 ROD INITIAL LIMITS — remaining=%1, dpLimit=%2")
                          .arg(remainingLength)
                          .arg(dpLimit));

            } else {
                zInfo("❌ No suitable stock rod found.");
            }




        } // stock ág vége

        //if (remainingLength == 0) { groupVec.removeFirst(); continue; }
        if (!rodSelected) {
            zError(QString("❌ NO ROD AVAILABLE: materialId=%1, pendingPieces=%2")
                       .arg(targetMaterialId.toString())
                       .arg(groupVec.size()));

            zError("❌ STOCK SCAN SUMMARY:");
            for (int i = 0; i < _inventorySnapshot.profileInventory.size(); ++i) {
                const auto &s = _inventorySnapshot.profileInventory[i];
                zError(QString("   • STOCK[%1]: materialId=%2, quantity=%3")
                           .arg(i)
                           .arg(s.materialId.toString())
                           .arg(s.quantity));
            }

            QStringList groupIdStrings2;
            for (const QUuid &id : groupedMaterialIds)
                groupIdStrings2 << id.toString();

            zError(QString("❌ GROUP IDS WERE: %1")
                       .arg(groupIdStrings2.join(", ")));

            break;
        }


        ++rodId; // új rúd

        zInfo(QString("🔎 RÚD‑LOOP INDÍTÁSA — rodId=%1, barcode=%2, length=%3, reusable=%4")
                  .arg(rod.rodId)
                  .arg(rod.barcode)
                  .arg(rod.length)
                  .arg(rod.isReusable));


        int rodloopcounter = 0;
        // 2/d. Rod‑loop stop feltételekkel
        while (true) {
            zInfo("ROD LOOP: #" + QString::number(rodloopcounter));

            RodStepResult stepResult = RodLoopEngine::step(
                groupVec,
                remainingLength,
                dpLimit,
                rod,
                machine,
                currentOpId,
                rodId,
                kerf_mm,
                *this);

            QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents, 1);

            if (stepResult == RodStepResult::ContinueSameRod) {
                continue;
            }

            if (stepResult == RodStepResult::StartNewRod) {
                // ugyanaz, mint a régi `continue` a külső while-ra:
                break;
            }

            if (stepResult == RodStepResult::StopRod) {
                break;
            }
        }// rod-loop vége

        zInfo("➡ RÚD‑LOOP LEZÁRVA");

        createPhysicalLeftover(rod, remainingLength, currentOpId);

        // UI yield
        QCoreApplication::processEvents(QEventLoop::AllEvents, 20);
        QThread::msleep(1);

    }
    zInfo("🟢 OPTIMALIZÁCIÓ BEFEJEZVE — nincs több pending darab");


    // A lokális leftoverokat commitoljuk a globális készletbe
    // A lokális leftoverokat commitoljuk a globális készletbe
    for (const auto& entry : _localLeftovers) {
        _inventorySnapshot.reusableInventory.append(entry);
        // zEvent(QString("📦 Commit leftover: %1 (%2 mm)")
        //            .arg(entry.barcode).arg(entry.availableLength_mm));
    }
    _localLeftovers.clear();

    // 3️⃣ Szegmens-szintű front trim utómunka (csak stock rudakra)
    for (auto& plan : _result_plans) {
        bool isStockRod = !plan.isReusable();   // vagy plan.source == Stock
        this->applyFrontTrimToPlan(plan.planId, plan.kerfUsed_mm, isStockRod);
    }

    // --- IDE JÖN A VÉGÉRE ---
    double ms = timer.elapsed();

    int rodCount   = _result_plans.size();
    int pieceCount = 0;
    for (const auto& p : _result_plans)
        pieceCount += p.piecesWithMaterial.size();

    const auto& t = _fitTelemetry;

    double avgPicked = (t.calls > 0)
                           ? double(t.totalPicked) / double(t.calls)
                           : 0.0;
    double avgWaste = (t.calls > 0)
                          ? double(t.totalWaste) / double(t.calls)
                          : 0.0;
    double avgKerf  = (t.calls > 0)
                         ? double(t.totalKerf) / double(t.calls)
                         : 0.0;
    double avgKerfRatio = (t.totalUsed > 0)
                              ? (double(t.totalKerf) / double(t.totalUsed)) * 100.0
                              : 0.0;

    double avgElapsed_ms = (t.calls > 0)
                               ? double(t.totalElapsed_us) / 1000.0 / double(t.calls)
                               : 0.0;
    double avgBF_ms = (t.bruteForce > 0)
                          ? double(t.totalBFElapsed_us) / 1000.0 / double(t.bruteForce)
                          : 0.0;
    double avgDP_ms = ((t.dpPlain + t.dpScoring) > 0)
                          ? double(t.totalDPPlainElapsed_us + t.totalDPScoringElapsed_us)
                                / 1000.0
                                / double(t.dpPlain + t.dpScoring)
                          : 0.0;

    double avgGreedy_ms = (t.greedy > 0)
                              ? double(t.totalGreedyElapsed_us) / 1000.0 / double(t.greedy)
                              : 0.0;

    double avgBFCombos = (t.bruteForce > 0)
                             ? double(t.bf_totalCombos) / double(t.bruteForce)
                             : 0.0;
    double avgBFEvaluated = (t.bruteForce > 0)
                                ? double(t.bf_totalEvaluated) / double(t.bruteForce)
                                : 0.0;
    double avgBFSkipped = (t.bruteForce > 0)
                              ? double(t.bf_totalSkipped) / double(t.bruteForce)
                              : 0.0;

    double avgDPLimit = ((t.dpPlain + t.dpScoring) > 0)
                            ? double(t.dp_totalLimit) / double(t.dpPlain + t.dpScoring)
                            : 0.0;
    double avgDPFilled = ((t.dpPlain + t.dpScoring) > 0)
                             ? double(t.dp_totalFilledCells) / double(t.dpPlain + t.dpScoring)
                             : 0.0;
    double avgDPChain = ((t.dpPlain + t.dpScoring) > 0)
                            ? double(t.dp_totalChainLength) / double(t.dpPlain + t.dpScoring)
                            : 0.0;

    double avgGreedySorted = (t.greedy > 0)
                                 ? double(t.greedy_totalSortedSize) / double(t.greedy)
                                 : 0.0;
    double avgGreedyPicks = (t.greedy > 0)
                                ? double(t.greedy_totalPicks) / double(t.greedy)
                                : 0.0;

    zInfo("📊 OPTIMALIZÁCIÓS TELEMETRIA — összegzés");
    zInfo(QString("   • Hívások száma: %1").arg(t.calls));
    zInfo(QString("   • FullFit: %1").arg(t.fullFit));
    zInfo(QString("   • BruteForce: %1").arg(t.bruteForce));
    zInfo(QString("   • DP (plain): %1").arg(t.dpPlain));
    zInfo(QString("   • DP (scoring): %1").arg(t.dpScoring));
    zInfo(QString("   • Greedy: %1").arg(t.greedy));


    zInfo(QString("   • Átlag pick count: %1").arg(avgPicked, 0, 'f', 2));
    zInfo(QString("   • Átlag waste: %1 mm").arg(avgWaste, 0, 'f', 2));
    zInfo(QString("   • Max waste: %1 mm").arg(t.maxWaste));
    zInfo(QString("   • Min waste: %1 mm").arg(t.minWaste));
    zInfo(QString("   • Átlag kerf: %1 mm").arg(avgKerf, 0, 'f', 2));
    zInfo(QString("   • Kerf arány: %1 %%").arg(avgKerfRatio, 0, 'f', 3));
    zInfo(QString("   • Átlag futási idő: %1 ms").arg(avgElapsed_ms, 0, 'f', 3));
    zInfo(QString("   • Átlag BF idő: %1 ms").arg(avgBF_ms, 0, 'f', 3));
    zInfo(QString("   • Átlag DP idő: %1 ms").arg(avgDP_ms, 0, 'f', 3));
    zInfo(QString("   • Átlag Greedy idő: %1 ms").arg(avgGreedy_ms, 0, 'f', 3));


    zInfo(QString("   • BF átlag kombinációk: %1").arg(avgBFCombos, 0, 'f', 2));
    zInfo(QString("   • BF átlag vizsgált: %1").arg(avgBFEvaluated, 0, 'f', 2));
    zInfo(QString("   • BF átlag skip: %1").arg(avgBFSkipped, 0, 'f', 2));
    zInfo(QString("   • DP átlag limit: %1").arg(avgDPLimit, 0, 'f', 2));
    zInfo(QString("   • DP átlag filled cells: %1").arg(avgDPFilled, 0, 'f', 2));
    zInfo(QString("   • DP átlag chain length: %1").arg(avgDPChain, 0, 'f', 2));
    zInfo(QString("   • Greedy átlag rendezett elemszám: %1").arg(avgGreedySorted, 0, 'f', 2));
    zInfo(QString("   • Greedy átlag picks: %1").arg(avgGreedyPicks, 0, 'f', 2));

    zInfo("📘 TELEMETRIA — vége");
    zEvent(QString("🟢 OPTIMALIZÁCIÓ KÉSZ — idő=%1 ms, rudak=%2, darabok=%3")
               .arg(ms, 0, 'f', 0)
               .arg(rodCount)
               .arg(pieceCount));


}

// void OptimizerModel::logCutState(const Cutting::Plan::CutPlan& p,
//                                  int remainingLengthBefore,
//                                  int remainingLengthAfter)
// {
//     zInfo(QString("CUT-STATE: rodId=%1, sourceBarcode=%2, totalLength=%3, "
//                   "pieces=%4, kerfTotal=%5, wasteField=%6, "
//                   "remainingBefore=%7, remainingAfter=%8")
//               .arg(p.rodId)
//               .arg(p.sourceBarcode)
//               .arg(p.totalLength)
//               .arg(p.piecesWithMaterial.size())
//               .arg(p.kerfTotal)
//               .arg(p.waste)
//               .arg(remainingLengthBefore)
//               .arg(remainingLengthAfter));
// }


CutResult OptimizerModel::commitCutResult(
    const CutResult& cr,
    int& remainingLength,
    int& dpLimit,
    const SelectedRod& rod,
    int currentOpId,
    QVector<Cutting::Piece::PieceWithMaterial>& groupVec, double kerf_mm)
{
    zInfo(QString("🔍 COMMIT CUT RESULT — rodId=%1, used=%2, waste=%3")
              .arg(rod.rodId)
              .arg(cr.used)
              .arg(cr.waste));


    if (cr.status == CutResultStatus::Overfill){
        zInfo("✖ COMMIT — overfill, nincs mentés");
        return cr;
    }

    zInfo(QString("🎯 COMMIT — plan mentve (planId=%1, pieces=%2)")
              .arg(cr.plan.planId.toString())
              .arg(cr.plan.piecesWithMaterial.size()));

    // 1️⃣ Result + Plan mentése
    _result_plans.append(cr.plan);
    _planned_leftovers.append(cr.result);

    zInfo(QString("   • COMMIT — %1 darab eltávolítva a pending listából")
              .arg(cr.usedPieceIds.size()));

    // 2️⃣ groupVec törlése
    for (auto id : cr.usedPieceIds) {
        groupVec.erase(
            std::remove_if(groupVec.begin(), groupVec.end(),
                           [&](const auto& c){ return c.info.pieceId == id; }),
            groupVec.end());
    }



    // 3️⃣ leftover lifecycle
    // if (!cr.result.isFinalWaste && cr.result.waste > 0) {
    //     LeftoverStockEntry entry;
    //     entry.materialId = cr.result.materialId;
    //     entry.availableLength_mm = cr.result.waste;
    //     entry.used = false;
    //     entry.barcode = cr.result.reusableBarcode;
    //     entry.parentBarcode = rod.barcode;
    //     entry.source = Cutting::Result::LeftoverSource::Optimization;
    //     entry.optimizationId = std::make_optional(currentOpId);

    //     leftoverRodMap.insert(entry.entryId, rod.rodId);
    //     _localLeftovers.append(entry);
    // }

    // 4️⃣ reusable tiltás
    if (rod.isReusable && rod.entryId.has_value())
        _usedLeftoverEntryIds.insert(rod.entryId.value());

    // 5️⃣ remainingLength frissítés
    // remainingLength  -= cr.used;
    // remainingLength2 -= cr.used;


    zInfo(QString("🟦 COMMIT BEFORE LIMITS — remaining=%1, dpLimit=%2")
              .arg(remainingLength)
              .arg(dpLimit));

    // 5️⃣ Fizikai maradék
    remainingLength -= cr.used;
    if (remainingLength < 0)
        remainingLength = 0;

    // 6️⃣ DP-limit újraszámolása
    if (rod.isReusable) {
        // leftover továbbvágása → nincs új end-trim
        dpLimit = remainingLength;
    } else {
        // stock rúd → új end-trim + minimum hulló
        dpLimit = remainingLength
                           - OptimizerConstants::END_TRIM_MM
                           - OptimizerConstants::MINIMUM_HULLO_MM
                           - OptimizerUtils::roundKerfLoss(1, kerf_mm);

        if (dpLimit < 0)
            dpLimit = 0;
    }

    zInfo(QString("🟦 COMMIT AFTER LIMITS — remaining=%1, dpLimit=%2")
              .arg(remainingLength)
              .arg(dpLimit));


    return cr;
}


CutResult OptimizerModel::cutSingle_AndCommit(
    const Cutting::Piece::PieceWithMaterial& piece,
    int& remainingLength,
    int& dpLimit,
    const SelectedRod& rod,
    const CuttingMachine& machine,
    int currentOpId,
    int rodId,
    double kerf_mm,
    QVector<Cutting::Piece::PieceWithMaterial>& groupVec)
{
    CutResult cr = CutEngine::cutSingle(
        piece,
        remainingLength,
        rod,
        machine,
        currentOpId,
        rodId,
        kerf_mm,
        planCounter
        );

    // QString planTxt = cr.plan.toLogEntry(machine);
    // zEvent(planTxt);

    return commitCutResult(cr, remainingLength, dpLimit, rod, currentOpId, groupVec, kerf_mm);
}

CutResult OptimizerModel::cutCombo_AndCommit(
    const QVector<Cutting::Piece::PieceWithMaterial>& combo,
    int& remainingLength,
    int& dpLimit,
    const SelectedRod& rod,
    const CuttingMachine& machine,
    int currentOpId,
    int rodId,
    double kerf_mm,
    QVector<Cutting::Piece::PieceWithMaterial>& groupVec)
{
    CutResult cr = CutEngine::cutCombo(
        combo,
        remainingLength,
        rod,
        machine,
        currentOpId,
        rodId,
        kerf_mm,
        planCounter
        );

    // QString planTxt = cr.plan.toLogEntry(machine);
    // zEvent(planTxt);

    return commitCutResult(cr, remainingLength, dpLimit, rod, currentOpId, groupVec, kerf_mm);
}



void OptimizerModel::setCuttingRequests(const QVector<Cutting::Plan::Request>& list) {
    _requests = list;
}


void OptimizerModel::applyFrontTrimToPlan(const QUuid& planId,
                                          double kerf_mm,
                                          bool isStockRod)
{
    zInfo(QString("🔍 FRONT TRIM — planId=%1, isStock=%2")
              .arg(planId.toString())
              .arg(isStockRod));

    // Csak stock rúdra alkalmazzuk
    if (!isStockRod)
        return;

    // 1️⃣ Megkeressük a megfelelő CutPlan‑t
    auto it = std::find_if(_result_plans.begin(), _result_plans.end(),
                           [&](const Cutting::Plan::CutPlan& p){
                               return p.planId == planId;
                           });
    if (it == _result_plans.end())
        return;

    Cutting::Plan::CutPlan& plan = *it;
    auto& segs = plan.segments;
    if (segs.isEmpty())
        return;

    // 2️⃣ Utolsó Waste szegmens (végmaradék)
    int lastWasteIx = -1;
    for (int i = segs.size() - 1; i >= 0; --i) {
        if (segs[i].isWaste()) {
            lastWasteIx = i;
            break;
        }
    }


    zInfo(QString("🔎 FRONT TRIM — utolsó waste index=%1").arg(lastWasteIx));

    if (lastWasteIx < 0)
        return;

    double frontTrim = OptimizerConstants::END_TRIM_MM; // 15 mm
    double frontKerf = kerf_mm;

    double delta = frontTrim + frontKerf;

    // Ha a végmaradék ennél kisebb, nem piszkáljuk (védőfék)
    if (segs[lastWasteIx].length_mm() <= delta) {
        zInfo(QString("✖ FRONT TRIM — waste túl kicsi (waste=%1 mm, delta=%2 mm)")
                  .arg(segs[lastWasteIx].length_mm())
                  .arg(delta));
        return;
    }

    // 3️⃣ Végmaradék rövidítése
    segs[lastWasteIx].shrinkLength(delta);

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

    segs.insert(0, frontKerfSeg);
    segs.insert(0, frontTech);

    // 5️⃣ Indexek újraszámozása típusonként
    int pieceIx = 1;
    int kerfIx  = 1;
    int wasteIx = 1;
    int techIx  = 1;

    for (auto& s : segs) {
        using T = Cutting::Segment::SegmentModel::Type;
        if (s.isPiece()) {
            s.setIndex(pieceIx++);
        }
        else if (s.isKerf()) {
            s.setIndex(kerfIx++);
        }
        else if (s.isWaste()) {
            s.setIndex(wasteIx++);
        }
        else if (s.isTechnical()) {
            s.setIndex(techIx++);
        }
    }

    zInfo("📊 FRONT TRIM — kész, indexek újraszámozva");

    // ⚖️ plan.waste, result.waste, leftover.availableLength_mm NEM változik
    // csak a waste szegmens eloszlását módosítottuk (eleje vs vége),
    // az össz‑hossz változatlan.
}


void OptimizerModel::createPhysicalLeftover(const SelectedRod& rod,
                                 int remainingLength,
                                 int currentOpId)
{
    zInfo(QString("🔍 FIZIKAI HULLÓ KERESÉSE — remaining=%1 mm").arg(remainingLength));
    if (remainingLength <= 0)
        return;

    // PATCH #10 — fizikai leftover modell
    int frontTrim = rod.isReusable ? 0 : OptimizerConstants::END_TRIM_MM;
    int backTrim = OptimizerConstants::END_TRIM_MM;        // 15 mm gyári vég
    int minHull  = OptimizerConstants::MINIMUM_HULLO_MM;   // 70 mm minimum hulló

    int corrected = remainingLength - frontTrim - backTrim - minHull;
    if (corrected <= 0) {
        zInfo(QString("✖ Fizikai hulló nem hozható létre — corrected=%1 mm").arg(corrected));
        return;
    }


    LeftoverStockEntry entry;
    entry.materialId = rod.materialId;
    entry.availableLength_mm = corrected;
    entry.used = false;
    entry.barcode = IdentifierUtils::makeLeftoverId(SettingsManager::instance().nextMaterialCounter());
    entry.parentBarcode = rod.barcode;
    entry.source = Cutting::Result::LeftoverSource::Optimization;
    entry.optimizationId = std::make_optional(currentOpId);

    zInfo(QString("🎯 Fizikai hulló létrehozva — barcode=%1, length=%2 mm, rodId=%3, parent=%4")
              .arg(entry.barcode)
              .arg(entry.availableLength_mm)
              .arg(rod.rodId)
              .arg(rod.barcode));

    leftoverRodMap.insert(entry.entryId, rod.rodId);
    _localLeftovers.append(entry);
}

} //end namespace Optimizer
} //end namespace Cutting
