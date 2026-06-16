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
#include "inventoryhelper.h"
#include "machineselecthelper.h"
#include "pendinganalyzer.h"
#include "piecebuilder.h"
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

    // 🔹 Csak globális snapshot készül – lokális pool külön marad
    QVector<LeftoverStockEntry> globalSnapshot = _inventorySnapshot.reusableInventory;

    InventoryHelper::logSnapshot(_inventorySnapshot.reusableInventory);
    QHash<QUuid, QVector<Cutting::Piece::PieceWithMaterial>> piecesByMaterial = PieceBuilder::buildPiecesByMaterial(_requests);

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
                machine.kerf_mm,
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
    }
    _localLeftovers.clear();

    // 3️⃣ Szegmens-szintű front trim utómunka (csak stock rudakra)
    for (auto& plan : _result_plans) {
        auto* m = MaterialRegistry::instance().findById(plan.materialId);
        MaterialTrimmingParams tp = m ? m->trimmingParams(plan.isReusable())
                                     : MaterialTrimmingParams::getDefault();
        SegmentPostProcess::applyFrontTrimToPlan(plan, tp);
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
        rod.length = best.stock.availableLength_mm;
        rod.isReusable = true;
        rod.barcode = best.stock.barcode;
        rod.entryId = best.stock.entryId;
        rod._parent = best.stock._parent;

        const MaterialMaster* material = MaterialRegistry::instance().findById(rod.materialId);
        MaterialTrimmingParams tp = material ? material->trimmingParams(rod.isReusable)
                                             : MaterialTrimmingParams::getDefault();


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

            zInfo(QString("🆔 NEW ROD ID: %1 (source=stock, material=%2, length=%3)")
                      .arg(rod.rodId)
                      .arg(material ? material->toDisplay() : rod.materialId.toString())
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
                  - tp.frontTrim_mm
                  - tp.minLeftOver_mm;

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

            const MaterialMaster* material = MaterialRegistry::instance().findById(rod.materialId);
            MaterialTrimmingParams tp = material ? material->trimmingParams(rod.isReusable)
                                                 : MaterialTrimmingParams::getDefault();

            dpLimit = rod.length
                      - tp.frontTrim_mm
                      - tp.backTrim_mm
                      - tp.minLeftOver_mm;

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

