#include <algorithm>
#include <QSet>
#include <QCoreApplication>
#include <QThread>
#include <QDebug>
#include <QElapsedTimer>

#include "storage/registry/storageregistry.h"

#include <cutting/snapshot/cuttingsnapshotdeserializer.h>
#include <cutting/snapshot/cuttingsnapshotserializer.h>

#include "cutengine.h"
#include "cuttypes.h"
#include "optimizermodel.h"
#include "model/cutting/optimizer/leftoverlifecycle.h"
#include "model/cutting/optimizer/lineagehelper.h"
#include "model/cutting/optimizer/segmentpostprocess.h"
#include "model/cutting/optimizer/telemetryhelper.h"
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
//#include "../../../common/settingsmanager.h"
#include "inventoryhelper.h"
#include "machineselecthelper.h"
#include "pendinganalyzer.h"
#include "piecebuilder.h"
#include "rodloopengine.h"


namespace Cutting {
namespace Optimizer {

OptimizerModel::OptimizerModel(QObject *parent) : QObject(parent) {}

const QVector<Cutting::Plan::CutPlan>& OptimizerModel::getResult_PlansRef() const {
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

    // 🔹 Csak globális snapshot készül – lokális pool külön marad
    QVector<LeftoverStockEntry> globalSnapshot = _inventorySnapshot.reusableInventory;

    //InventoryHelper::logSnapshot(_inventorySnapshot.reusableInventory);
    QHash<QUuid, QVector<Cutting::Piece::PieceWithMaterial>> piecesByMaterial =
        PieceBuilder::buildPiecesByMaterial(_requests, _inventorySnapshot);

    // --- piecesByMaterial summary (optimalizált log) ---
    zInfo("OptimizerModel: piecesByMaterial summary:");

    for (auto it = piecesByMaterial.begin(); it != piecesByMaterial.end(); ++it) {

        auto *mat = MaterialRegistry::instance().findById(it.key());
        QString matName = mat ? mat->barcode : "?";

        const auto& vec = it.value();

        zInfo(QString("  → material=%1 count=%2")
                  .arg(matName)
                  .arg(vec.size()));

        for (const auto& p : vec) {

            QString src = p.info.leftoverEntryId.has_value()
            ? "leftover"
            : "stock";

            zInfo(QString("     • len=%1 role=%2 src=%3 extRef=%4")
                      .arg(p.info.length_mm)
                      .arg(ToldasRoleUtils::toDisplayText(p.info.toldasRole))
                      .arg(src)
                      .arg(p.info.externalReference));
        }
    }


    // --- TOLDAS_MAIN darabok whole-cutolása (optimalizált blokk) ---
    {
        for (auto it = piecesByMaterial.begin(); it != piecesByMaterial.end(); ++it) {

            auto& vec = it.value();

            // gép kiválasztása egyszer anyagonként
            auto ma = MachineUtils::pickMachineForMaterial(it.key());

            vec.erase(std::remove_if(vec.begin(), vec.end(),
                                     [&](const auto& p){

                                         if (p.info.toldasRole != ToldasRole::Main)
                                             return false;

                                         // Whole-cut plan generálása rúd nélkül
                                         auto crWhole = CutEngine::cutWhole(
                                             p,
                                             *ma,
                                             currentOpId,
                                             planCounter,
                                             ++rodCounter);

                                         _result_plans.append(crWhole.plan);

                                         if (p.info.leftoverEntryId.has_value()) {
                                             QUuid loId = *p.info.leftoverEntryId;

                                             // globalSnapshot-ból törlés
                                             globalSnapshot.erase(
                                                 std::remove_if(globalSnapshot.begin(), globalSnapshot.end(),
                                                                [&](const LeftoverStockEntry& e){
                                                                    return e.entryId == loId;
                                                                }),
                                                 globalSnapshot.end());

                                             // _inventorySnapshot.reusableInventory-ből törlés
                                             _inventorySnapshot.reusableInventory.erase(
                                                 std::remove_if(_inventorySnapshot.reusableInventory.begin(),
                                                                _inventorySnapshot.reusableInventory.end(),
                                                                [&](const LeftoverStockEntry& e){
                                                                    return e.entryId == loId;
                                                                }),
                                                 _inventorySnapshot.reusableInventory.end());

                                             // opcionálisan: tiltás az _usedLeftoverEntryIds-ben
                                             _usedLeftoverEntryIds.insert(loId);

                                             // törlés a lokális leftover poolból is
                                             _localLeftovers.erase(
                                                 std::remove_if(_localLeftovers.begin(), _localLeftovers.end(),
                                                                [&](const LeftoverStockEntry& e){ return e.entryId == loId; }),
                                                 _localLeftovers.end());
                                         }

                                         zInfo(QString("Whole-cut TOLDAS_MAIN: len=%1 extRef=%2")
                                                   .arg(p.info.length_mm)
                                                   .arg(p.info.externalReference));

                                         return true; // töröljük a pending listából
                                     }),
                      vec.end());
        }
    }

    int rodId = 0;
    static int counter = 0;

    auto anyPending = [&]() {
        for (auto it = piecesByMaterial.begin(); it != piecesByMaterial.end(); ++it)
            if (!it.value().isEmpty()) return true;
        return false;
    };

    zInfo("🔍 OPTIMALIZÁCIÓ INDÍTÁSA — pending darabok keresése");
    // 2. Optimalizációs ciklus
    while (anyPending()) {
        zInfo(QString("🔎 OPTIMIZER LOOP #%1 — pending darabok vizsgálata").arg(counter));

        // FAILED darabok kiszűrése (helyes hely!)
        for (auto it = piecesByMaterial.begin(); it != piecesByMaterial.end(); ++it) {
            auto &vec = it.value();   // NEM const → módosítható
            vec.erase(std::remove_if(vec.begin(), vec.end(),
                                     [](const auto& p){ return p.failed; }),
                      vec.end());
        }

        auto stats = PendingAnalyzer::analyze(piecesByMaterial, heuristic);
        QUuid targetMaterialId = stats.targetMaterialId;

        if (counter % 50 == 0) {            
            QCoreApplication::processEvents(QEventLoop::AllEvents, 20);
        }
        counter++;

        auto &groupVec = piecesByMaterial[targetMaterialId];
        if (groupVec.isEmpty()) {
            zInfo("✖ Nincs több darab ehhez az anyagcsoporthoz — továbblépés");
            continue;
        }

        zInfo(QString("✔ Aktuális anyagcsoport pending: %1 db").arg(groupVec.size()));

        // 2/b. Gép kiválasztása
        std::optional<CuttingMachine> machineOpt =
            MachineSelectHelper::pickAndLog(targetMaterialId); // adhatna pointert, tisztább lenne mint az opt
        if (!machineOpt) { groupVec.removeFirst(); continue; }
        const CuttingMachine machine = *machineOpt;

        auto init = initRodForMaterial(
            targetMaterialId,
            globalSnapshot,
            groupVec,
            machine.kerf_mm);

        // if (!init.ok) {
        //     auto* mat = MaterialRegistry::instance().findById(targetMaterialId);
        //     QString targetMaterialName = mat?mat->toDisplay():"?";
        //     zWarning(QString("❌ NO ROD AVAILABLE: materialId=%1, pendingPieces=%2")
        //                .arg(targetMaterialName)
        //                .arg(groupVec.size()));
        //     break;
        // }
        auto* mat = MaterialRegistry::instance().findById(targetMaterialId);
        QString targetMaterialName = mat ? mat->toDisplay() : "?";

        if (!init.ok) {


            zWarning(QString("❌ NO ROD AVAILABLE — material=%1, pendingPieces=%2")
                         .arg(targetMaterialName)
                         .arg(groupVec.size()));

            // 🔥 PATCH — minden darabot FAILED-re jelölünk
            for (auto &p : groupVec) {

                p.failed = true;
                p.failReason = QString("Nincs megfelelő rúd a vágáshoz");

                // gép ID nem ismert → 0 vagy egy default gép
                DiscardedPiece dp;
                dp.requestId  = p.info.requestId;
                dp.materialId = p.materialId;
                dp.machineId  = machine.id;
                dp.failReason = p.failReason;
                dp.pieceId    = p.info.pieceId;

                addDiscardedPiece(dp);

                zWarning("❌ FAILED PIECE_2 — " + dp.failReason);
            }

            // töröljük a pendingből
            groupVec.clear();

            // folytatjuk a következő anyagcsoporttal
            continue;
        }

        SelectedRod rod = init.rod;
        int remainingLength = init.remainingLength;
        int dpLimit = init.dpLimit;

        ++rodId;

        zInfo(QString("🔎 RÚD‑LOOP INDÍTÁSA — rodId=%1, barcode=%2, length=%3, reusable=%4")
                  .arg(rod.rodId)
                  .arg(rod.barcode)
                  .arg(rod.length)
                  .arg(rod.isReusable?"REUSABLE":"STOCK_ROD"));

        int rodloopcounter = 0;
        // 2/d. Rod‑loop stop feltételekkel
        while (true)
        {
            zInfo("ROD LOOP: #" + QString::number(rodloopcounter));

            //auto* mat = MaterialRegistry::instance().findById(rod.materialId);
            zInfo(QString("🔧 ROD AUDIT — rodId=%1, material=%2, barcode=%3, length=%4")
                      .arg(rod.rodId)
                      .arg(targetMaterialName)
                      .arg(rod.barcode)
                      .arg(rod.length));

            zInfo(QString("🔗 ROD LINEAGE — rodId=%1, parent=%2")
                      .arg(rod.rodId)
                      .arg(rod._parent ? rod._parent->toString() : "—"));

            zInfo(QString("🔧 ROD LIMITS — remaining=%1, dpLimit=%2")
                      .arg(remainingLength)
                      .arg(dpLimit));

            zInfo(QString("🔧 MATERIAL GROUP — rodMaterial=%1, groupSize=%2")
                      .arg(targetMaterialName)
                      .arg(groupVec.size()));

            zInfo(QString("🔧 ROD ORIGIN — reusable=%1, barcode=%2")
                      .arg(rod.isReusable?"REUSABLE":"STOCK_ROD")
                      .arg(rod.entryId.has_value() ? rod.barcode : "—"));

            RodLoopEngine::RodStepResultModel stepResult = RodLoopEngine::step(
                groupVec,
                remainingLength,
                dpLimit,
                rod,
                machine,
                currentOpId,
                rodId,
                machine.kerf_mm,
                *this);

            QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents, 1);

            if (stepResult.rodStepResult == RodLoopEngine::RodStepResult::ContinueSameRod) {
                // ❗ PATCH: fizikai ellenőrzés — ha remainingLength == rod.length → új rúd kell
                // if (remainingLength == rod.length) {
                //     zWarning("⛔ PATCH — fizikai képtelenség: a rúd teljes hosszú, mégis ContinueSameRod jött → új rúd indítása");
                //     break; // új rúd
                // }

                // semmi rod újrainicializálás
                // semmi új rúd keresés
                // csak folytatjuk a rod-loopot a friss remainingLength/dpLimit értékekkel
                rodloopcounter++;
                continue;
            }

            // if (stepResult.rodStepResult == RodLoopEngine::RodStepResult::StartNewRod) {
            //     // ugyanaz, mint a régi `continue` a külső while-ra:
            //     break;
            // }

            if (stepResult.rodStepResult == RodLoopEngine::RodStepResult::StartNewRod) {
                // új rúd indítása → új rodId
                QString newRodId = IdentifierUtils::makeRodId(++rodCounter);

                // új stock rúd kiválasztása
                rod = selectStockRod(stepResult.materialId, newRodId, /*barcode*/ rod.barcode);

                break;
            }

            // if (stepResult.rodStepResult == RodLoopEngine::RodStepResult::StartNewStockRod) {
            //     int matId = SettingsManager::instance().nextMaterialCounter();
            //     QString barcode = IdentifierUtils::makeMaterialId(matId);

            //     rod = selectStockRod(stepResult.materialId, rod.rodId, barcode);   // ← csak stock
            //     continue;  // új rúd-loop
            // }
            // if (stepResult.rodStepResult == RodLoopEngine::RodStepResult::StartNewStockRod) {
            //     int matId = SettingsManager::instance().nextMaterialCounter();
            //     QString barcode = IdentifierUtils::makeMaterialId(matId);

            //     SelectedRod newRod = selectStockRod(stepResult.materialId,
            //                                         IdentifierUtils::makeRodId(++rodCounter),
            //                                         barcode);

            //     rod = newRod;
            //     continue;
            // }

            if (stepResult.rodStepResult == RodLoopEngine::RodStepResult::StartNewStockRod) {
                int matId = SettingsManager::instance().nextMaterialCounter();
                QString barcode = IdentifierUtils::makeMaterialId(matId);

                // ÚJ rodId generálása
                QString newRodId = IdentifierUtils::makeRodId(++rodCounter);

                rod = selectStockRod(stepResult.materialId, newRodId, barcode);
                continue;
            }

            if (stepResult.rodStepResult == RodLoopEngine::RodStepResult::StopRod) {
                break;
            }
        }// rod-loop vége

        zInfo("➡ RÚD‑LOOP LEZÁRVA");

        //createPhysicalLeftover(rod, remainingLength, currentOpId);

        bool created = false;
        auto entry = LeftoverLifecycle::createPhysicalLeftover(
            rod,
            remainingLength,
            currentOpId,
            _result_plans,
            created
            );

        if (created) {
            leftoverRodMap.insert(entry.entryId, RodLineage{ rod.rodId, rod._parent });
            _localLeftovers.append(entry);
        }

        // UI yield
        QCoreApplication::processEvents(QEventLoop::AllEvents, 20);
        QThread::msleep(1);

    }
    zInfo("🟢 OPTIMALIZÁCIÓ BEFEJEZVE — nincs több pending darab");

    // A lokális leftoverokat commitoljuk a globális készletbe
    for (const auto& entry : _localLeftovers) {
        _inventorySnapshot.reusableInventory.append(entry);
    }
    _localLeftovers.clear();

    // // FAILED darabok logolása
    // int failedCount = 0;
    // for (auto it = piecesByMaterial.begin(); it != piecesByMaterial.end(); ++it) {
    //     for (const auto& p : it.value()) {
    //         if (p.failed) {
    //             failedCount++;
    //             zWarning("❌ VÁGÁSI HIBA — " + p.failReason);
    //         }
    //     }
    // }

    // 🔥 PATCH — FAILED darabok logolása kizárólag a DiscardedPiece alapján
    int failedCount = 0;
    for (const auto& dp : _discardedPieces.values()) {
        failedCount++;
        zWarning("❌ VÁGÁSI HIBA — " + dp.failReason);
    }

    // 3️⃣ Szegmens-szintű front trim utómunka (csak stock rudakra)
    // for (Plan::CutPlan &plan : _result_plans) {
    //     auto *m = MaterialRegistry::instance().findById(plan.materialId);
    //     MaterialTrimmingParams tp = m ? m->trimmingParams(plan.isReusable())
    //                                   : MaterialTrimmingParams::getDefault();
    //     SegmentPostProcess::applyFrontTrimToPlan(plan, tp);
    // }
    // 3️⃣ Szegmens-szintű front trim utómunka — csak vágott rudakra
    for (Plan::CutPlan &plan : _result_plans) {

        // ha minden darab whole-cut, akkor ez a rúd technikailag nincs vágva → skip
        const bool allWhole =
            std::all_of(plan.piecesWithMaterial.begin(),
                        plan.piecesWithMaterial.end(),
                        [](const Cutting::Piece::PieceWithMaterial &pw) {
                            return pw.info.keepWhole;
                        });

        if (allWhole)
            continue;

        auto *m = MaterialRegistry::instance().findById(plan.materialId);
        MaterialTrimmingParams tp = m ? m->trimmingParams(plan.isReusable())
                                      : MaterialTrimmingParams::getDefault();

        SegmentPostProcess::applyFrontTrimToPlan(plan, tp);
    }

    // --- Globális darabszám összegzés + eltérés riport ---
    double ms = timer.elapsed();

    int rodCount   = _result_plans.size();
    int pieceCount = 0;
    for (const auto& p : _result_plans)
        pieceCount += p.piecesWithMaterial.size();

    // ⚠️ REQUEST vs CUT eltérés riportálása
    {
        int totalRequested = _requests.size();   // összes igényelt darab
        int totalCut       = pieceCount;         // ténylegesen levágott darabok
        int totalFailed    = failedCount;        // FAILED darabok száma

        if (totalRequested > totalCut) {
            zEvent(QString("⚠️ Figyelem: %1 darab kérve, %2 darab levágva, %3 darab nem teljesíthető.")
                       .arg(totalRequested)
                       .arg(totalCut)
                       .arg(totalFailed));
        }
    }

    TelemetryHelper::logSummary(_fitTelemetry);

    // 🟦 MachineReport feltöltése (expectedPieces)
    _machineReport.clear();

    for (const auto& plan : _result_plans) {
        auto& rep = _machineReport[plan.machineId];
        rep.machineId = plan.machineId;

        const CuttingMachine* m =
            CuttingMachineRegistry::instance().findById(plan.machineId);
        rep.machineName = m ? m->name : "?";

        rep.expectedPieces += plan.piecesWithMaterial.size();
    }

    // --- gépenkénti FAILED darabszám beírása ---
    for (const auto& dp : _discardedPieces.values()) {
        auto& rep = _machineReport[dp.machineId];
        rep.failedPieces_Local += 1;
    }

    // 🔍 GLOBAL CUT AUDIT
    {
        int failedCount = _discardedPieces.size();
        int totalPlans  = _result_plans.size();

        bool anyError = false;

        // Rod ID egyediség ellenőrzése
        QSet<QString> rodIds;
        bool rodIdDuplicate = false;

        for (const auto& plan : _result_plans) {
            if (rodIds.contains(plan.rodId)) {
                rodIdDuplicate = true;
                anyError = true;
                zWarning(QString("❌ AUDIT — DUPLICATE rodId detected: %1")
                             .arg(plan.rodId));
            }
            rodIds.insert(plan.rodId);
        }

        if (!rodIdDuplicate) {
            zInfo("🟢 AUDIT — rodId uniqueness OK");
        }

        // 🔍 Fizikai túlvágás audit — leftoverok alapján
        bool physicalOvercut = false;

        for (const auto& entry : _localLeftovers) {
            if (entry.availableLength_mm < 0) {
                physicalOvercut = true;
                anyError = true;
                zWarning(QString("❌ AUDIT — physical overcut detected in leftover=%1 (len=%2)")
                             .arg(entry.barcode)
                             .arg(entry.availableLength_mm));
            }
        }

        if (!physicalOvercut) {
            zInfo("🟢 AUDIT — no physical overcut detected");
        }

        // FAILED darabok összegzése
        if (failedCount > 0) {
            anyError = true;
            zWarning(QString("❌ AUDIT — %1 darab FAILED").arg(failedCount));
        } else {
            zInfo("🟢 AUDIT — no FAILED pieces");
        }

        // 🔍 AUDIT — Request teljesülés (toldás-aware)
        {
            bool reqError = false;

            struct ReqInfo {
                int needCount = 0;
            };

            QHash<QUuid, ReqInfo> reqInfos;

            // 1️⃣ Request → extRef-ek előkészítése
            for (const auto& req : _requests) {
                reqInfos[req.requestId].needCount += req.quantity;
            }

            // 2️⃣ Levágott darabok összegyűjtése
            struct CutInfo {
                int mainCount   = 0;
                int toldatCount = 0;
                int normalCount = 0;
            };

            QHash<QUuid, CutInfo> cutInfos;

            for (const auto& plan : _result_plans) {
                for (const auto& p : plan.piecesWithMaterial) {

                    QUuid reqId = p.info.requestId;

                    // 🔧 toldásos darabok normalizálása
                    if (!reqInfos.contains(reqId))
                        continue;

                    auto& ci = cutInfos[reqId];

                    switch (p.info.toldasRole) {
                    case ToldasRole::Main:
                        ci.mainCount++;
                        break;
                    case ToldasRole::Toldat:
                        ci.toldatCount++;
                        break;
                    default:
                        ci.normalCount++;
                        break;
                    }
                }
            }

            // 3️⃣ Audit — darab teljesülés számítása
            for (auto it = reqInfos.begin(); it != reqInfos.end(); ++it) {

                QUuid reqId = it.key();
                const ReqInfo& ri = it.value();
                const CutInfo ci = cutInfos.value(reqId);

                int fulfilled = 0;

                // toldásos darabok: 1 darab = 1 main + 1 toldat
                int toldasPairs = std::min(ci.mainCount, ci.toldatCount);
                fulfilled += toldasPairs;

                // sima darabok
                fulfilled += ci.normalCount;

                if (fulfilled != ri.needCount) {
                    reqError = true;
                    anyError = true;

                    auto *r = CuttingPlanRequestRegistry::instance().findById(reqId);
                    QString rName = r?r->toString():"???";
                    zWarning(QString("❌ AUDIT — Request %1 mismatch: need=%2 got=%3 "
                                     "(normal=%4 main=%5 toldat=%6)")
                                 .arg(rName)
                                 .arg(ri.needCount)
                                 .arg(fulfilled)
                                 .arg(ci.normalCount)
                                 .arg(ci.mainCount)
                                 .arg(ci.toldatCount));
                }
            }

            // 🔍 4️⃣ Felesleg detektálása
            for (auto it = cutInfos.begin(); it != cutInfos.end(); ++it) {
                QUuid reqId = it.key();
                if (!reqInfos.contains(reqId)) {
                    anyError = true;
                    reqError = true;

                    auto *r = CuttingPlanRequestRegistry::instance().findById(reqId);
                    QString rName = r?r->toString():"???";

                    zWarning(
                        QString("❌ AUDIT — Extra cut piece found without matching request: reqId=%1")
                                 .arg(rName));
                }
            }

            // 5️⃣ Összegzés
            if (!reqError)
                zEvent("🟢 AUDIT — All requests fully satisfied (toldás-aware)");
            else
                zEvent("❌ AUDIT — Request mismatch detected (toldás included)");
        }



        // 🔔 Globális audit eredmény
        if (anyError) {
            zEvent("❌ GLOBAL CUT AUDIT — hibák találhatók, vágás nem biztonságos!");
        } else {
            zEvent("🟢 GLOBAL CUT AUDIT — minden rendben, vágás biztonságos.");
        }
    }

    zEvent(QString("🟢 OPTIMALIZÁCIÓ KÉSZ — idő=%1 ms, rudak=%2, darabok=%3")
               .arg(ms, 0, 'f', 0)
               .arg(rodCount)
               .arg(pieceCount));

}


CutResult OptimizerModel::commitCutResult(
    CutResult& cr,
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


    // PATCH 11 — leftover fogyasztás auditálása
    // Ha a darab leftoverből jött, akkor a leftover készletből el kell távolítani.
    // A PieceInfo tartalmazza a leftoverEntryId-t.

    for (auto id : cr.usedPieceIds) {
        // megkeressük a darabot a commit előtt
        auto it = std::find_if(groupVec.begin(), groupVec.end(),
                               [&](const auto& c){ return c.info.pieceId == id; });

        if (it != groupVec.end()) {
            const auto& piece = *it;

            if (piece.info.leftoverEntryId.has_value()) {
                QUuid consumedId = piece.info.leftoverEntryId.value();

                zInfo(QString("♻ Leftover fogyasztás — entryId=%1")
                          .arg(consumedId.toString()));

                // Globális reusableInventory módosítása
                auto& inv = _inventorySnapshot.reusableInventory;

                inv.erase(
                    std::remove_if(inv.begin(), inv.end(),
                                   [&](const LeftoverStockEntry& e){
                                       return e.entryId == consumedId;
                                   }),
                    inv.end());
            }
        }
    }

    // 4️⃣ reusable tiltás
    if (rod.isReusable && rod.entryId.has_value())
        _usedLeftoverEntryIds.insert(rod.entryId.value());


    zInfo(QString("🟦 COMMIT BEFORE LIMITS — remaining=%1, dpLimit=%2")
              .arg(remainingLength)
              .arg(dpLimit));

    // 5️⃣ Fizikai maradék
    remainingLength -= cr.used;
    if (remainingLength < 0)
        remainingLength = 0;

    dpLimit -= cr.used;
    if (dpLimit < 0)
        dpLimit = 0;

    zInfo(QString("🟦 COMMIT AFTER LIMITS — remaining=%1, dpLimit=%2")
              .arg(remainingLength)
              .arg(dpLimit));

    LineageHelper::validateLineage(cr.plan, _result_plans);
    zInfo(LineageHelper::lineageTree(cr.plan, _result_plans));

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
        kerf_mm,dpLimit,
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
        kerf_mm,dpLimit,
        planCounter
        );

    // QString planTxt = cr.plan.toLogEntry(machine);
    // zEvent(planTxt);

    return commitCutResult(cr, remainingLength, dpLimit, rod, currentOpId, groupVec, kerf_mm);
}

void OptimizerModel::setCuttingRequests(const QVector<Cutting::Plan::Request>& list) {
    _requests = list;
}

// RodInitResult OptimizerModel::initRodForMaterial(
//     const QUuid& targetMaterialId,
//     QVector<LeftoverStockEntry>& globalSnapshot,
//     QVector<Cutting::Piece::PieceWithMaterial>& groupVec,
//     double kerf_mm)
// {
//     RodInitResult out;

//     zInfo("🔍 RÚD KERESÉSE — először hulló, majd stock vizsgálata");

//     // 1️⃣ merged snapshot
//     auto merged = globalSnapshot;
//     merged += _localLeftovers;

//     // 2️⃣ reusable keresés
//     std::optional<ReusableCandidate> candidate;

//     if (_useReusableLeftovers) {
//         candidate =
//             ReusableFitEngine::findBestReusableFit(
//                 merged,
//                 globalSnapshot.size(),
//                 groupVec,
//                 targetMaterialId,
//                 kerf_mm,
//                 _usedLeftoverEntryIds,
//                 *this);
//     }
//     SelectedRod rod;
//     int remainingLength = 0;
//     int dpLimit = 0;
//     bool rodSelected = false;

//     // 3️⃣ reusable ág
//     if (candidate.has_value()
//         && (candidate->stock.source == Cutting::Result::LeftoverSource::Optimization
//             || candidate->stock.source == Cutting::Result::LeftoverSource::Manual))
//     {
//         const auto& best = *candidate;

//         _usedLeftoverEntryIds.insert(best.stock.entryId);

//         rod.materialId = best.stock.materialId;
//         rod.length = best.stock.availableLength_mm;
//         rod.isReusable = true;
//         rod.barcode = best.stock.barcode;
//         rod.entryId = best.stock.entryId;
//         rod._parent = best.stock._parent;

//         const MaterialMaster* material = MaterialRegistry::instance().findById(rod.materialId);
//         MaterialTrimmingParams tp = material ? material->trimmingParams(rod.isReusable)
//                                              : MaterialTrimmingParams::getDefault();


//         // rodId mapping
//         if (leftoverRodMap.contains(best.stock.entryId)) {
//             auto lineage = leftoverRodMap.value(best.stock.entryId);
//             rod.rodId = lineage.rodId;
//             rod._parent = lineage.parent;

//             zInfo(QString("REUSE ROD FROM LEFTOVER: rodId=%1, stock=%2")
//                       .arg(rod.rodId)
//                       .arg(best.stock.barcode));
//         } else {
//             rod.rodId = IdentifierUtils::makeRodId(++rodCounter);

//             zInfo(QString("🆔 NEW ROD ID: %1 (source=stock, material=%2, length=%3)")
//                       .arg(rod.rodId)
//                       .arg(material ? material->toDisplay() : rod.materialId.toString())
//                       .arg(rod.length));

//             leftoverRodMap.insert(best.stock.entryId, RodLineage{ rod.rodId, rod._parent });

//             zInfo(QString("MAP-INSERT (on select): %1 → %2 (parent=%3)")
//                       .arg(best.stock.barcode)
//                       .arg(rod.rodId)
//                       .arg(rod._parent ? rod._parent->toString() : "—"));
//         }

//         // snapshot törlés
//         if (best.source == ReusableCandidate::Source::GlobalSnapshot) {
//             globalSnapshot.erase(
//                 std::remove_if(globalSnapshot.begin(), globalSnapshot.end(),
//                                [&](const LeftoverStockEntry& e) {
//                                    return e.entryId == best.stock.entryId;
//                                }),
//                 globalSnapshot.end());
//         } else {
//             _localLeftovers.erase(
//                 std::remove_if(_localLeftovers.begin(), _localLeftovers.end(),
//                                [&](const LeftoverStockEntry& e) {
//                                    return e.entryId == best.stock.entryId;
//                                }),
//                 _localLeftovers.end());
//         }

//         remainingLength = rod.length;


//         dpLimit = rod.length
//                   - tp.frontTrim_mm
//                   - tp.minLeftOver_mm;

//         rodSelected = true;

//         zInfo(QString("SELECTED REUSABLE ROD: rodId=%1, barcode=%2, length=%3")
//                   .arg(rod.rodId)
//                   .arg(rod.barcode)
//                   .arg(rod.length));
//     }

//     // 4️⃣ stock fallback
//     if (!rodSelected)
//     {
//         zInfo("♻️ No reusable leftover fits — falling back to stock.");

//         QSet<QUuid> groupIds = GroupUtils::groupMembers(targetMaterialId);

//         auto stockRod = StockFitEngine::pickStockRod(
//             _inventorySnapshot.profileInventory,
//             groupIds,
//             rodCounter);

//         if (stockRod.has_value()) {
//             rod = *stockRod;
//             rod._parent = std::nullopt;

//             remainingLength = rod.length;

//             const MaterialMaster* material = MaterialRegistry::instance().findById(rod.materialId);
//             MaterialTrimmingParams tp = material ? material->trimmingParams(rod.isReusable)
//                                                  : MaterialTrimmingParams::getDefault();

//             dpLimit = rod.length
//                       - tp.frontTrim_mm
//                       - tp.backTrim_mm
//                       - tp.minLeftOver_mm;

//             rodSelected = true;

//             zInfo(QString("🟦 ROD SELECTED — rodId=%1, barcode=%2, length=%3, reusable=%4, entryId=%5")
//                       .arg(rod.rodId)
//                       .arg(rod.barcode)
//                       .arg(rod.length)
//                       .arg(rod.isReusable)
//                       .arg(rod.entryId.has_value() ? rod.entryId->toString() : "—"));

//             zInfo(QString("🟦 ROD INITIAL LIMITS — remaining=%1, dpLimit=%2")
//                       .arg(remainingLength)
//                       .arg(dpLimit));
//         } else {
//             zInfo("❌ No suitable stock rod found.");
//         }
//     }

//     // 5️⃣ nincs rúd → fail
//     if (!rodSelected) {
//         out.ok = false;
//         return out;
//     }

//     // 6️⃣ siker → kitöltjük az eredményt
//     out.ok = true;
//     out.rod = rod;
//     out.remainingLength = remainingLength;
//     out.dpLimit = dpLimit;
//     return out;
// }
RodInitResult OptimizerModel::initRodForMaterial(
    const QUuid& targetMaterialId,
    QVector<LeftoverStockEntry>& globalSnapshot,
    QVector<Cutting::Piece::PieceWithMaterial>& groupVec,
    double kerf_mm)
{
    RodInitResult out;

    zInfo("🔍 RÚD KERESÉSE — először hulló, majd stock vizsgálata");



    // 1️⃣ merged snapshot
    auto merged = globalSnapshot;
    merged += _localLeftovers;

    // 2️⃣ reusable keresés
    std::optional<ReusableCandidate> candidate;


    if (_useReusableLeftovers) {
        candidate =
            ReusableFitEngine::findBestReusableFit(
                merged,
                globalSnapshot.size(),
                groupVec,
                targetMaterialId,
                kerf_mm,
                _usedLeftoverEntryIds,
                *this);
    }

    SelectedRod rod;
    int remainingLength = 0;
    int dpLimit = 0;
    bool rodSelected = false;

    // 3️⃣ reusable ág
    if (candidate.has_value()
        && (candidate->stock.source == Cutting::Result::LeftoverSource::Optimization
            || candidate->stock.source == Cutting::Result::LeftoverSource::Manual))
    {
        const auto& best = *candidate;

        _usedLeftoverEntryIds.insert(best.stock.entryId);

        rod.materialId = best.stock.materialId;
        rod.length     = best.stock.availableLength_mm;
        rod.isReusable = true;
        rod.barcode    = best.stock.barcode;
        rod.entryId    = best.stock.entryId;
        rod._parent    = best.stock._parent;

        // // ❗ PATCH #1 — fizikai hossz ellenőrzése
        // int needed = groupVec.first().info.length_mm + static_cast<int>(kerf_mm);
        // if (rod.length < needed) {
        //     zInfo(QString("♻️ PATCH — leftover túl rövid (rod=%1, needed=%2), fallback stock")
        //               .arg(rod.length)
        //               .arg(needed));
        //     rodSelected = false;
        //     // → korai fallback
        //     goto STOCK_FALLBACK;
        // }

        const MaterialMaster* material =
            MaterialRegistry::instance().findById(rod.materialId);
        MaterialTrimmingParams tp = material
                                         ? material->trimmingParams(rod.isReusable)
                                         : MaterialTrimmingParams::getDefault();

        dpLimit = rod.length
                  - tp.frontTrim_mm
                  - tp.backTrim_mm
                  - tp.minLeftOver_mm;

        // ❗ PATCH #2 — dpLimit ellenőrzése
        //dpLimit = rod.length - tp.frontTrim_mm - tp.minLeftOver_mm;
        // if (dpLimit < groupVec.first().info.length_mm) {
        //     zInfo("♻️ PATCH — dpLimit túl kicsi leftover rúdhoz, fallback stock");
        //     rodSelected = false;
        //     goto STOCK_FALLBACK;
        // }

        // rodId mapping
        if (leftoverRodMap.contains(best.stock.entryId)) {
            auto lineage = leftoverRodMap.value(best.stock.entryId);
            rod.rodId    = lineage.rodId;

            rod._parent  = lineage.parent;

            zInfo(QString("REUSE ROD FROM LEFTOVER: rodId=%1, stock=%2")
                      .arg(rod.rodId)
                      .arg(best.stock.barcode));
        } else {
            rod.rodId = IdentifierUtils::makeRodId(++rodCounter);

            zInfo(QString("🆔 NEW ROD ID: %1 (source=leftover, material=%2, length=%3)")
                      .arg(rod.rodId)
                      .arg(material ? material->toDisplay() : rod.materialId.toString())
                      .arg(rod.length));

            leftoverRodMap.insert(best.stock.entryId, RodLineage{ rod.rodId, rod._parent });

            zInfo(QString("MAP-INSERT (on select): %1 → %2 (parent=%3)")
                      .arg(best.stock.barcode)
                      .arg(rod.rodId)
                      .arg(rod._parent ? rod._parent->toString() : "—"));
        }

        // ❗ PATCH #3 — leftover törlése csak ha tényleg használjuk
        if (best.source == ReusableCandidate::Source::GlobalSnapshot) {
            globalSnapshot.erase(
                std::remove_if(globalSnapshot.begin(), globalSnapshot.end(),
                               [&](const LeftoverStockEntry& e) {
                                   return e.entryId == best.stock.entryId;
                               }),
                globalSnapshot.end());
        } else {
            _localLeftovers.erase(
                std::remove_if(_localLeftovers.begin(), _localLeftovers.end(),
                               [&](const LeftoverStockEntry& e) {
                                   return e.entryId == best.stock.entryId;
                               }),
                _localLeftovers.end());
        }

        remainingLength = rod.length;
        rodSelected = true;

        // ⚠️ ANYAGCSERE RIPORT — leftover használata
        {
            const MaterialMaster* origMat =
                MaterialRegistry::instance().findById(targetMaterialId);
            const MaterialMaster* chosenMat =
                MaterialRegistry::instance().findById(rod.materialId);

            if (origMat && chosenMat) {
                zEvent(QString(
                           "⚠️ Anyagvariáns váltás — a(z) %1 helyett leftover szál került felhasználásra "
                           "(barcode=%2, length=%3 mm)"
                           )
                           .arg(origMat->barcode)
                           .arg(rod.barcode)
                           .arg(rod.length));
            }
        }

        zInfo(QString("SELECTED REUSABLE ROD: rodId=%1, barcode=%2, length=%3")
                  .arg(rod.rodId)
                  .arg(rod.barcode)
                  .arg(rod.length));
        zInfo(QString("🔧 ROD INIT AUDIT (REUSABLE) — rodId=%1, barcode=%2, parent=%3, length=%4")
                  .arg(rod.rodId)
                  .arg(rod.barcode)
                  .arg(rod._parent ? rod._parent->toString() : "—")
                  .arg(rod.length));

    }

// 4️⃣ stock fallback
//STOCK_FALLBACK:
    if (!rodSelected)
    {
        zInfo("♻️ No reusable leftover fits — falling back to stock.");

        // --- PATCH #2: fizikai ellenőrzés stock fallback előtt ---
        int needed = groupVec.isEmpty()
                         ? 0
                         : groupVec.first().info.length_mm + static_cast<int>(kerf_mm);

        const MaterialMaster* matStock =
            MaterialRegistry::instance().findById(targetMaterialId);

        int stockLen = matStock ? matStock->stockLength_mm : 0;

        if (needed > stockLen) {
            zWarning(QString("⛔ PATCH#2 — darab nem fér fel stock rúdra sem: needed=%1, stockLen=%2")
                         .arg(needed)
                         .arg(stockLen));

            // Nem indítunk új rudat → visszatérünk FAIL-lel
            out.ok = false;
            return out;
        }
        // --- PATCH END ---

        QSet<QUuid> groupIds = GroupUtils::groupMembers(targetMaterialId);


        std::optional<SelectedRod> stockRod2 =
            StockFitEngine::pickStockRod2(
                _inventorySnapshot.profileInventory,
                groupIds,
                rodCounter,
                groupVec.isEmpty() ? 0 : groupVec.first().info.length_mm,
                kerf_mm);

        if (stockRod2.has_value()) {
            QString oldRodId = rod.rodId;   // audit
            rod = *stockRod2;
            rod._parent = std::nullopt;

            zInfo(QString("🔧 ROD INIT AUDIT (STOCK v2) — oldRodId=%1 → newRodId=%2, barcode=%3, length=%4")
                      .arg(oldRodId)
                      .arg(rod.rodId)
                      .arg(rod.barcode)
                      .arg(rod.length));

            remainingLength = rod.length;

            const MaterialMaster* material =
                MaterialRegistry::instance().findById(rod.materialId);

            MaterialTrimmingParams tp = material
                                            ? material->trimmingParams(rod.isReusable)
                                            : MaterialTrimmingParams::getDefault();

            dpLimit = rod.length
                      - tp.frontTrim_mm
                      - tp.backTrim_mm
                      - tp.minLeftOver_mm;

            rodSelected = true;

            zInfo(QString("🟦 ROD SELECTED (v2) — rodId=%1, barcode=%2, length=%3, reusable=%4")
                      .arg(rod.rodId)
                      .arg(rod.barcode)
                      .arg(rod.length)
                      .arg(rod.isReusable));

            zInfo(QString("🟦 ROD INITIAL LIMITS (v2) — remaining=%1, dpLimit=%2")
                      .arg(remainingLength)
                      .arg(dpLimit));
        }

        if (!rodSelected) {
            std::optional<SelectedRod> stockRod =
                StockFitEngine::pickStockRod(
                    _inventorySnapshot.profileInventory,
                    groupIds,
                    rodCounter);

            if (stockRod.has_value()) {
                // ⚠️ ANYAGCSERE RIPORT — UI-ba
                const MaterialMaster* origMat =
                    MaterialRegistry::instance().findById(targetMaterialId);
                const MaterialMaster* chosenMat =
                    MaterialRegistry::instance().findById(stockRod->materialId);

                if (origMat && chosenMat) {
                    zEvent(QString(
                               "⚠️ Anyagvariáns váltás — a csoportban a(z) %1 helyett "
                               "a(z) %2 szál lett kiválasztva (length=%3 mm)"
                               )
                               .arg(origMat->barcode)
                               .arg(chosenMat->barcode)
                               .arg(chosenMat->stockLength_mm));
                }
                QString oldRodId = rod.rodId;
                rod = *stockRod;
                rod._parent = std::nullopt;

                // ÚJ rodId minden stock rúdhoz
                QString newRodId = IdentifierUtils::makeRodId(++rodCounter);
                rod.rodId = newRodId;

                zInfo(QString("🔧 ROD INIT AUDIT (STOCK fallback) — oldRodId=%1 → newRodId=%2, barcode=%3, length=%4")
                          .arg(oldRodId)
                          .arg(newRodId)
                          .arg(rod.barcode)
                          .arg(rod.length));

                remainingLength = rod.length;

                const MaterialMaster* material =
                    MaterialRegistry::instance().findById(rod.materialId);

                MaterialTrimmingParams tp = material
                                                ? material->trimmingParams(rod.isReusable)
                                                : MaterialTrimmingParams::getDefault();

                dpLimit = rod.length
                          - tp.frontTrim_mm
                          - tp.backTrim_mm
                          - tp.minLeftOver_mm;

                rodSelected = true;

                zInfo(QString("🟦 ROD SELECTED (fallback) — rodId=%1, barcode=%2, length=%3, reusable=%4, entryId=%5")
                          .arg(rod.rodId)
                          .arg(rod.barcode)
                          .arg(rod.length)
                          .arg(rod.isReusable)
                          .arg(rod.entryId.has_value() ? rod.entryId->toString() : "—"));

                zInfo(QString("🟦 ROD INITIAL LIMITS — remaining=%1, dpLimit=%2")
                          .arg(remainingLength)
                          .arg(dpLimit));
            }
            else
            {
                zInfo("❌ No suitable stock rod found.");
            }
        }
    }


    // 5️⃣ nincs rúd → fail
    if (!rodSelected) {
        out.ok = false;
        return out;
    }

    // 6️⃣ siker → kitöltjük az eredményt
    out.ok            = true;
    out.rod           = rod;
    out.remainingLength = remainingLength;
    out.dpLimit       = dpLimit;
    return out;
}

SelectedRod OptimizerModel::selectStockRod(QUuid materialId, const QString& rodid, const QString& rodBarcode)
{
    const MaterialMaster* mat = MaterialRegistry::instance().findById(materialId);
    if (!mat) {
        QString msg = QString("❗ selectStockRod — ismeretlen materialId=%1").arg(materialId.toString());
        zWarning(msg);
        return SelectedRod(); // üres rúd, de ez ritka
    }

    SelectedRod rod;
    rod.materialId = materialId;
    rod.length     = mat->stockLength_mm;
    rod.isReusable = false;
    rod.origin     = RodOrigin::Stock;

    // A stock rúd barcode-ja → ez kerül a cutinstructionsbe
    rod.barcode    = rodBarcode;   // külső címke
    rod.rodId      = rodid;          // belső identitás

    rod.entryId    = std::nullopt;   // leftover only
    rod._parent    = std::nullopt;   // leftover only

    zInfo(QString("📦 STOCK RÚD VÁLASZTVA — material=%1, barcode=%2, length=%3")
              .arg(mat->name)
              .arg(rod.barcode)
              .arg(rod.length));

    return rod;
}


void OptimizerModel::saveSnapshot(const QString& filePath)
{
    QFile f(filePath);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text))
        return;

    QTextStream out(&f);

    CuttingSnapshotSerializer::writeSnapshot(out, _result_plans);
}

//////////////////////////////////////
/// \brief OptimizerModel::loadSnapshot
/// \param filePath
/// \return
///
bool OptimizerModel::loadSnapshot(const QString& filePath)
{
    QFile f(filePath);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
        return false;

    QTextStream in(&f);

    auto plans = CuttingSnapshotDeserializer::loadSnapshot(in);

    if (plans.isEmpty()){
        _result_plans.clear();
        return true;
    }

    _result_plans = plans;

    //emit snapshotLoaded();
    return true;
}


} //end namespace Optimizer
} //end namespace Cutting

