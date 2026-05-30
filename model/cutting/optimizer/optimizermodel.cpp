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
                  .arg(e._parent.has_value()
                           ? QString("parent=%1").arg(e._parent->toString())
                           : "parent=—"));

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

            //int sumLen = OptimizerUtils::sumLengths(it.value());
            auto info = OptimizerUtils::computePhysicalCut(it.value(), 0.0, INT_MAX);
            double sumLen = info.totalCut;

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
        double bestMetric = -1;
        for (auto it = piecesByMaterial.begin(); it != piecesByMaterial.end(); ++it) {
            double metric = 0;

            if (heuristic == TargetHeuristic::ByCount) {
                metric = it.value().size();
            } else {
                auto info = OptimizerUtils::computePhysicalCut(it.value(), 0.0, INT_MAX);
                metric = info.totalCut;   // vagy static_cast<int>(info.totalCut)
            }

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

        auto init = initRodForMaterial(
            targetMaterialId,
            globalSnapshot,
            groupVec,
            kerf_mm);

        if (!init.ok) {
            zError(QString("❌ NO ROD AVAILABLE: materialId=%1, pendingPieces=%2")
                       .arg(targetMaterialId.toString())
                       .arg(groupVec.size()));
            break;
        }

        SelectedRod rod = init.rod;
        int remainingLength = init.remainingLength;
        int dpLimit = init.dpLimit;

        ++rodId;

        zInfo(QString("🔎 RÚD‑LOOP INDÍTÁSA — rodId=%1, barcode=%2, length=%3, reusable=%4")
                  .arg(rod.rodId)
                  .arg(rod.barcode)
                  .arg(rod.length)
                  .arg(rod.isReusable));


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
                // semmi rod újrainicializálás
                // semmi új rúd keresés
                // csak folytatjuk a rod-loopot a friss remainingLength/dpLimit értékekkel
                rodloopcounter++;
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
    // A lokális leftoverokat commitoljuk a globális készletbe
    for (const auto& entry : _localLeftovers) {
        _inventorySnapshot.reusableInventory.append(entry);
        // zEvent(QString("📦 Commit leftover: %1 (%2 mm)")
        //            .arg(entry.barcode).arg(entry.availableLength_mm));
    }
    _localLeftovers.clear();

    // 3️⃣ Szegmens-szintű front trim utómunka (csak stock rudakra)
    for (auto& plan : _result_plans) {
        //bool isStockRod = !plan.isReusable();   // vagy plan.source == Stock
        //this->applyFrontTrimToPlan( plan.planId, plan.machineKerf, isStockRod);
        SegmentPostProcess::applyFrontTrimToPlan(plan);
    }

    // --- IDE JÖN A VÉGÉRE ---
    double ms = timer.elapsed();

    int rodCount   = _result_plans.size();
    int pieceCount = 0;
    for (const auto& p : _result_plans)
        pieceCount += p.piecesWithMaterial.size();

    TelemetryHelper::logSummary(_fitTelemetry);

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
    std::optional<ReusableCandidate> candidate =
        ReusableFitEngine::findBestReusableFit(
            merged,
            globalSnapshot.size(),
            groupVec,
            targetMaterialId,
            kerf_mm,
            _usedLeftoverEntryIds,
            *this);

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
        rod.length = best.stock.availableLength_mm;
        rod.isReusable = true;
        rod.barcode = best.stock.barcode;
        rod.entryId = best.stock.entryId;
        rod._parent = best.stock._parent;

        // rodId mapping
        if (leftoverRodMap.contains(best.stock.entryId)) {
            auto lineage = leftoverRodMap.value(best.stock.entryId);
            rod.rodId = lineage.rodId;
            rod._parent = lineage.parent;

            zInfo(QString("REUSE ROD FROM LEFTOVER: rodId=%1, stock=%2")
                      .arg(rod.rodId)
                      .arg(best.stock.barcode));
        } else {
            rod.rodId = IdentifierUtils::makeRodId(++rodCounter);

            const MaterialMaster* m = MaterialRegistry::instance().findById(rod.materialId);
            zInfo(QString("🆔 NEW ROD ID: %1 (source=stock, material=%2, length=%3)")
                      .arg(rod.rodId)
                      .arg(m ? m->toDisplay() : rod.materialId.toString())
                      .arg(rod.length));

            leftoverRodMap.insert(best.stock.entryId, RodLineage{ rod.rodId, rod._parent });

            zInfo(QString("MAP-INSERT (on select): %1 → %2 (parent=%3)")
                      .arg(best.stock.barcode)
                      .arg(rod.rodId)
                      .arg(rod._parent ? rod._parent->toString() : "—"));
        }

        // snapshot törlés
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

        dpLimit = rod.length
                  - OptimizerConstants::END_TRIM_MM
                  - OptimizerConstants::MINIMUM_HULLO_MM;

        rodSelected = true;

        zInfo(QString("SELECTED REUSABLE ROD: rodId=%1, barcode=%2, length=%3")
                  .arg(rod.rodId)
                  .arg(rod.barcode)
                  .arg(rod.length));
    }

    // 4️⃣ stock fallback
    if (!rodSelected)
    {
        zInfo("♻️ No reusable leftover fits — falling back to stock.");

        QSet<QUuid> groupIds = GroupUtils::groupMembers(targetMaterialId);

        auto stockRod = StockFitEngine::pickStockRod(
            _inventorySnapshot.profileInventory,
            groupIds,
            rodCounter);

        if (stockRod.has_value()) {
            rod = *stockRod;
            rod._parent = std::nullopt;

            remainingLength = rod.length;

            dpLimit = rod.length
                      - OptimizerConstants::END_TRIM_MM
                      - OptimizerConstants::END_TRIM_MM
                      - OptimizerConstants::MINIMUM_HULLO_MM;

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
    }

    // 5️⃣ nincs rúd → fail
    if (!rodSelected) {
        out.ok = false;
        return out;
    }

    // 6️⃣ siker → kitöltjük az eredményt
    out.ok = true;
    out.rod = rod;
    out.remainingLength = remainingLength;
    out.dpLimit = dpLimit;
    return out;
}


} //end namespace Optimizer
} //end namespace Cutting

