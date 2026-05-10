#include <algorithm>
#include <QSet>
#include <QCoreApplication>
#include <QThread>
#include <QDebug>

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


    zEvent(QString("🔄 Optimize(%2) run started (heuristic=%1)")
               .arg(heuristic == TargetHeuristic::ByCount ? "ByCount" : "ByTotalLength")
               .arg(currentOpId));

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

    // auto mergeView = [&](const QVector<LeftoverStockEntry>& globalSnap,
    //                      const QVector<LeftoverStockEntry>& localPool) {
    //     QVector<LeftoverStockEntry> view = globalSnap;
    //     view += localPool; // csak olvasási célra fésüljük össze
    //     return view;
    // };

    // 1. Anyagigény - a Darabok előkészítése anyag szerint
    QHash<QUuid, QVector<Cutting::Piece::PieceWithMaterial>> piecesByMaterial;
    for (const Cutting::Plan::Request &req : _requests) {
        for (int i = 0; i < req.quantity; ++i) {
            Cutting::Piece::PieceInfo info;
            info.length_mm = req.requiredLength;
            //info.ownerName = req.ownerName.isEmpty() ? "Ismeretlen" : req.ownerName;
            //info.externalReference = req.externalReference;
            info.requestId = req.requestId;
            info.isCompleted = false;
            piecesByMaterial[req.materialId].append(
                Cutting::Piece::PieceWithMaterial(info, req.materialId));
        }
    }

    auto anyPending = [&]() {
        for (auto it = piecesByMaterial.begin(); it != piecesByMaterial.end(); ++it)
            if (!it.value().isEmpty()) return true;
        return false;
    };

    int rodId = 0;

    static int counter = 0;

    // 2. Optimalizációs ciklus
    while (anyPending()) {

        if (counter % 50 == 0) {
            zInfo("optCounter: " + QString::number(counter));
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
        if (groupVec.isEmpty()) continue;

        // 2/b. Gép kiválasztása
        auto machineOpt = MachineUtils::pickMachineForMaterial(targetMaterialId);
        if (!machineOpt) { groupVec.removeFirst(); continue; }
        const CuttingMachine machine = *machineOpt;

        const double kerf_mm = machine.kerf_mm;

        //2/c. Rúd kiválasztása

        zInfo(QString("🔍 MATERIAL SELECT: targetMaterialId=%1, pendingPieces=%2")
                  .arg(targetMaterialId.toString())
                  .arg(groupVec.size()));

        QSet<QUuid> groupIds = GroupUtils::groupMembers(targetMaterialId);
        QStringList groupIdStrings;
        for (const QUuid &id : groupIds)
            groupIdStrings << id.toString();

        zInfo(QString("🔍 GROUP IDS FOR MATERIAL: %1")
                  .arg(groupIdStrings.join(", ")));


        SelectedRod rod;


        int remainingLength = 0;
        int remainingLength2 = 0;
        bool rodSelected = false;

        // Összefésült nézet készítése
        auto merged = globalSnapshot;
        merged += _localLeftovers;

        // ♻️ Először próbáljunk reusable-t
        // REUSEABLE ÁG
        std::optional<ReusableCandidate> candidate =
            ReusableFitEngine::findBestReusableFit(merged,globalSnapshot.size(), groupVec,targetMaterialId, kerf_mm, _usedLeftoverEntryIds);
        if (candidate.has_value() &&
            candidate->stock.source == Cutting::Result::LeftoverSource::Optimization)

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
                zInfo(QString("REUSE ROD FROM LEFTOVER: entryId=%1, rodId=%2, barcode=%3")
                          .arg(best.stock.entryId.toString())
                          .arg(rod.rodId)
                          .arg(best.stock.barcode));
            } else {
                rod.rodId = IdentifierUtils::makeRodId(++rodCounter);
                zWarning(QString("NEW ROD FOR LEFTOVER: entryId=%1, rodCounter=%2, rodId=%3, barcode=%4")
                             .arg(best.stock.entryId.toString())
                             .arg(rodCounter)
                             .arg(rod.rodId)
                             .arg(best.stock.barcode));

                // zWarning(QString("⚠️ Missing rodId mapping for leftover %1, new rodId=%2")
                //           .arg(best.stock.entryId.toString())
                //           .arg(rod.rodId));
                // 🔑 Azonnal regisztráljuk, hogy a lánc ne szakadjon meg
                leftoverRodMap.insert(best.stock.entryId, rod.rodId);
                zInfo(QString("MAP-INSERT (on select): %1 → %2")
                          .arg(best.stock.entryId.toString())
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

            remainingLength2 = rod.length
                               - OptimizerConstants::END_TRIM_MM        // gyári vég
                               - OptimizerConstants::MINIMUM_HULLO_MM   // mechanikai minimum
                               - OptimizerUtils::roundKerfLoss(1, kerf_mm);

            rodSelected = true;
            // reusable ág végén
            zInfo(QString("SELECTED REUSABLE ROD: rodId=%1, barcode=%2, length=%3, entryId=%4")
                      .arg(rod.rodId)
                      .arg(rod.barcode)
                      .arg(rod.length)
                      .arg(rod.entryId ? rod.entryId->toString() : "∅"));
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
                remainingLength2 = rod.length
                                   - OptimizerConstants::END_TRIM_MM        // front trim
                                   - OptimizerConstants::END_TRIM_MM        // gyári vég
                                   - OptimizerConstants::MINIMUM_HULLO_MM   // mechanikai minimum
                                   - OptimizerUtils::roundKerfLoss(1, kerf_mm);

                rodSelected = true;

                zInfo(QString("SELECTED STOCK ROD: rodId=%1, barcode=%2, length=%3")
                          .arg(rod.rodId)
                          .arg(rod.barcode)
                          .arg(rod.length));
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
            for (const QUuid &id : groupIds)
                groupIdStrings2 << id.toString();

            zError(QString("❌ GROUP IDS WERE: %1")
                       .arg(groupIdStrings2.join(", ")));

            break;
        }


        ++rodId; // új rúd

        zInfo(QString("ROD-LOOP START: rodId=%1, barcode=%2, length=%3, isReusable=%4, entryId=%5")
                  .arg(rod.rodId)
                  .arg(rod.barcode)
                  .arg(rod.length)
                  .arg(rod.isReusable)
                  .arg(rod.entryId ? rod.entryId->toString() : "∅"));

        // 2/d. Rod‑loop stop feltételekkel
        while (true) {

            RodStepResult stepResult = RodLoopEngine::step(
                groupVec,
                remainingLength,
                remainingLength2,
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
        createPhysicalLeftover(rod, remainingLength, currentOpId);

        // UI yield
        QCoreApplication::processEvents(QEventLoop::AllEvents, 20);
        QThread::msleep(1);

    }



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
}

void OptimizerModel::logCutState(const Cutting::Plan::CutPlan& p,
                                 int remainingLengthBefore,
                                 int remainingLengthAfter)
{
    zInfo(QString("CUT-STATE: rodId=%1, sourceBarcode=%2, totalLength=%3, "
                  "pieces=%4, kerfTotal=%5, wasteField=%6, "
                  "remainingBefore=%7, remainingAfter=%8")
              .arg(p.rodId)
              .arg(p.sourceBarcode)
              .arg(p.totalLength)
              .arg(p.piecesWithMaterial.size())
              .arg(p.kerfTotal)
              .arg(p.waste)
              .arg(remainingLengthBefore)
              .arg(remainingLengthAfter));
}


CutResult OptimizerModel::commitCutResult(
    const CutResult& cr,
    int& remainingLength,
    int& remainingLength2,
    const SelectedRod& rod,
    int currentOpId,
    QVector<Cutting::Piece::PieceWithMaterial>& groupVec)
{
    if (cr.status == CutResultStatus::Overfill)
        return cr;

    // 1️⃣ Result + Plan mentése
    _result_plans.append(cr.plan);
    _planned_leftovers.append(cr.result);

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
    remainingLength  -= cr.used;
    remainingLength2 -= cr.used;

    return cr;
}


CutResult OptimizerModel::cutSingle_AndCommit(
    const Cutting::Piece::PieceWithMaterial& piece,
    int& remainingLength,
    int& remainingLength2,
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

    QString planTxt = cr.plan.toLogEntry(machine);
    zEvent(planTxt);

    return commitCutResult(cr, remainingLength, remainingLength2, rod, currentOpId, groupVec);
}

CutResult OptimizerModel::cutCombo_AndCommit(
    const QVector<Cutting::Piece::PieceWithMaterial>& combo,
    int& remainingLength,
    int& remainingLength2,
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

    QString planTxt = cr.plan.toLogEntry(machine);
    zEvent(planTxt);

    return commitCutResult(cr, remainingLength, remainingLength2, rod, currentOpId, groupVec);
}



void OptimizerModel::setCuttingRequests(const QVector<Cutting::Plan::Request>& list) {
    _requests = list;
}


void OptimizerModel::applyFrontTrimToPlan(const QUuid& planId,
                                          double kerf_mm,
                                          bool isStockRod)
{
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
        if (segs[i].type() == Cutting::Segment::SegmentModel::Type::Waste) {
            lastWasteIx = i;
            break;
        }
    }
    if (lastWasteIx < 0)
        return;

    double frontTrim = OptimizerConstants::END_TRIM_MM; // 15 mm
    double frontKerf = kerf_mm;

    double delta = frontTrim + frontKerf;

    // Ha a végmaradék ennél kisebb, nem piszkáljuk (védőfék)
    if (segs[lastWasteIx].length_mm() <= delta)
        return;

    // 3️⃣ Végmaradék rövidítése
    segs[lastWasteIx].shrinkLength(delta);

    // 4️⃣ Elejére beszúrjuk: [Technical(15)] + [Kerf(frontKerf)]
    Cutting::Segment::SegmentModel frontTech(
        Cutting::Segment::SegmentModel::Type::Technical,
        frontTrim,
        /*ix*/ 0,
        QUuid()
        );

    Cutting::Segment::SegmentModel frontKerfSeg(
        Cutting::Segment::SegmentModel::Type::Kerf,
        frontKerf,
        /*ix*/ 0,
        QUuid()
        );

    segs.insert(0, frontKerfSeg);
    segs.insert(0, frontTech);

    // 5️⃣ Indexek újraszámozása típusonként
    int pieceIx = 1;
    int kerfIx  = 1;
    int wasteIx = 1;
    int techIx  = 1;

    for (auto& s : segs) {
        using T = Cutting::Segment::SegmentModel::Type;
        switch (s.type()) {
        case T::Piece:
            s.setIndex(pieceIx++);
            break;
        case T::Kerf:
            s.setIndex(kerfIx++);
            break;
        case T::Waste:
            s.setIndex(wasteIx++);
            break;
        case T::Technical:
            s.setIndex(techIx++);
            break;
        }
    }

    // ⚖️ plan.waste, result.waste, leftover.availableLength_mm NEM változik
    // csak a waste szegmens eloszlását módosítottuk (eleje vs vége),
    // az össz‑hossz változatlan.
}


void OptimizerModel::createPhysicalLeftover(const SelectedRod& rod,
                                 int remainingLength,
                                 int currentOpId)
{
    if (remainingLength <= 0)
        return;

    // PATCH #10 — fizikai leftover modell
    int frontTrim = rod.isReusable ? 0 : OptimizerConstants::END_TRIM_MM;
    int backTrim = OptimizerConstants::END_TRIM_MM;        // 15 mm gyári vég
    int minHull  = OptimizerConstants::MINIMUM_HULLO_MM;   // 70 mm minimum hulló

    int corrected = remainingLength - frontTrim - backTrim - minHull;
    if (corrected <= 0)
        return;

    LeftoverStockEntry entry;
    entry.materialId = rod.materialId;
    entry.availableLength_mm = corrected;
    entry.used = false;
    entry.barcode = IdentifierUtils::makeLeftoverId(SettingsManager::instance().nextMaterialCounter());
    entry.parentBarcode = rod.barcode;
    entry.source = Cutting::Result::LeftoverSource::Optimization;
    entry.optimizationId = std::make_optional(currentOpId);

    leftoverRodMap.insert(entry.entryId, rod.rodId);
    _localLeftovers.append(entry);
}

} //end namespace Optimizer
} //end namespace Cutting
