#include "optimizermodel.h"
#include "../../../common/eventlogger.h"
#include "../../../common/logger.h"
#include "materials/utils/material_group_utils.h"
#include "../../machine/machineutils.h"
#include "../../../service/cutting/optimizer/optimizerutils.h"
//#include "service/cutting/segment/segmentutils.h"
//#include "model/profilestock.h"
//#include <numeric>
#include <algorithm>
#include <QSet>
#include <QCoreApplication>
#include <QThread>

#include "materials/registry/material_registry.h"
#include <QDebug>
#include "../../../common/identifierutils.h"
#include "../../../common/settingsmanager.h"

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

        if (counter % 10 == 0) {
            zInfo("optCounter: " + QString::number(counter));
            QCoreApplication::processEvents(QEventLoop::AllEvents, 5);
            QThread::msleep(1); // 10 ms szünet minden 100 iterációban, hogy ne blokkoljuk a UI-t
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
            findBestReusableFit(merged, globalSnapshot.size(), groupVec,targetMaterialId, kerf_mm);
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
            //remainingLength2 = rod.length;
            // Reusable leftover: nincs gyári sérült vég → nincs 2×15 mm levonás            
            remainingLength2 = rod.length - OptimizerConstants::END_TRIM_MM - OptimizerConstants::MINIMUM_HULLO_MM;
            rodSelected = true;
            // reusable ág végén
            zInfo(QString("SELECTED REUSABLE ROD: rodId=%1, barcode=%2, length=%3, entryId=%4")
                      .arg(rod.rodId)
                      .arg(rod.barcode)
                      .arg(rod.length)
                      .arg(rod.entryId ? rod.entryId->toString() : "∅"));
        }
        else
        //STOCK ÁG
        {
            zInfo("♻️ No reusable leftover fits — falling back to stock.");

            // 🧱 Ha nincs, akkor stockból - Stock vizsgálata — ANYAGCSOPORT ALAPÚ
            QSet<QUuid> groupIds = GroupUtils::groupMembers(
                targetMaterialId); // már használod máshol is
            //for (auto &stock : _inventorySnapshot.profileInventory) {

            zInfo(QString("🔍 STOCK SCAN START: targetMaterialId=%1, stockCount=%2")
                      .arg(targetMaterialId.toString())
                      .arg(_inventorySnapshot.profileInventory.size()));

            for (int i = 0; i < _inventorySnapshot.profileInventory.size(); ++i) {
                StockEntry &stock = _inventorySnapshot.profileInventory[i];

                zInfo(QString("   • STOCK[%1]: materialId=%2, quantity=%3")
                          .arg(i)
                          .arg(stock.materialId.toString())
                          .arg(stock.quantity));


                if (!groupIds.contains(stock.materialId)) {
                    zInfo(QString("     ⛔ SKIP STOCK[%1]: materialId not in groupIds").arg(i));
                    continue;
                }
                if (stock.quantity <= 0) {
                    zInfo(QString("     ⛔ SKIP STOCK[%1]: quantity=0").arg(i));
                    continue;
                }

                zInfo(QString("     ✅ STOCK[%1] SELECTED: materialId=%2, newQuantity=%3")
                          .arg(i)
                          .arg(stock.materialId.toString())
                          .arg(stock.quantity - 1));

                stock.quantity--;
                rod.materialId = stock.materialId; // ← lehet MÁS, mint targetMaterialId, de
                // csoporttag
                rod.length = stock.master() ? stock.master()->stockLength_mm : 0;

                rod.isReusable = false;

                // 🔑 Új, mesterséges stock barcode generálása
                int matId = SettingsManager::instance().nextMaterialCounter();
                rod.barcode = IdentifierUtils::makeMaterialId(matId);

                zInfo(QString("🆕 Assigned MAT barcode=%1 for stock materialId=%2")
                          .arg(rod.barcode)
                          .arg(rod.materialId.toString()));

                // 🔑 Stabil emberi azonosító
                rod.rodId = IdentifierUtils::makeRodId(++rodCounter);

                zInfo(QString("NEW STOCK ROD: rodCounter=%1, rodId=%2, materialId=%3, barcode=%4")
                          .arg(rodCounter)
                          .arg(rod.rodId)
                          .arg(rod.materialId.toString())
                          .arg(rod.barcode));

                // Stock rúd: gyári sérült végek miatt 2×15 mm levonás
                remainingLength = rod.length;
                remainingLength2 = rod.length - 2 * OptimizerConstants::END_TRIM_MM - OptimizerConstants::MINIMUM_HULLO_MM;
                rodSelected = true;

                // stock ág végén
                zInfo(QString("SELECTED STOCK ROD: rodId=%1, barcode=%2, length=%3")
                          .arg(rod.rodId)
                          .arg(rod.barcode)
                          .arg(rod.length));

                break;
            }
        }

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
            QVector<Cutting::Piece::PieceWithMaterial> combo =
                findBestFit(groupVec, remainingLength2, kerf_mm);

            zInfo(QString("findBestFit: %1 darab, bestCombo size=%2")
                      .arg(groupVec.size())
                      .arg(combo.size()));
            if (combo.isEmpty())
                break;

            auto cutStatus1 =
                cutComboBatch(combo, remainingLength, rod, machine, currentOpId,
                                            rodId, kerf_mm, groupVec);
            if (cutStatus1 == CutStatus::Overfill) {
                // próbáljunk single-piece fallbacket
                std::optional<Cutting::Piece::PieceWithMaterial> single =
                    OptimizerUtils::findSingleBestPiece(groupVec, remainingLength2, kerf_mm);
                if (single.has_value()) {
                    auto cutStatus2 =
                        cutSinglePieceBatch(*single, remainingLength, rod, machine,
                                                          currentOpId, rodId, kerf_mm, groupVec);
                    // ha ez is overfill → akkor tényleg lezárjuk
                    if (cutStatus2 == CutStatus::Overfill) {
                        remainingLength = 0;
                        remainingLength2 = 0;
                        break;
                    }

                    // PATCH #27 — remainingLength frissítése single-piece után
                    int used = single->info.length_mm +
                               OptimizerUtils::roundKerfLoss(1, kerf_mm);
                    remainingLength -= used;

                    // ha sikerült → folytatjuk a rod-loopot
                    // fallback sikeres → rúd lezárása (NEM folytatjuk ugyanazon a
                    // rúdon)
                    // continue;
                    remainingLength = 0;
                    remainingLength2 = 0;
                    break;
                }

                // ha single-piece sem megy → lezárjuk
                remainingLength = 0;
                remainingLength2 = 0;
                break;
            } else {
                // PATCH #27 — remainingLength frissítése combo után
                int used = OptimizerUtils::sumLengths(combo) +
                           OptimizerUtils::roundKerfLoss(combo.size(), kerf_mm);
                remainingLength -= used;
                remainingLength2 -= used;


                    // 🔧 IDE jöhet a szegmens-utókezelés az utoljára létrejött CutPlan-re
            }



            // Selejt alá esés → lezárás
            if (remainingLength < OptimizerConstants::SELEJT_THRESHOLD) {
                // if (remainingLength > 0) {
                //     LeftoverStockEntry entry;
                //     entry.materialId = rod.materialId;
                //     entry.availableLength_mm = remainingLength;
                //     entry.used = false;
                //     entry.barcode = QString("UU1-%1").arg(QUuid::createUuid().toString().mid(1, 6));
                //     _localLeftovers.append(entry);
                // }
                break;
            }

            // Jó leftover tartomány (500–800) → lezárás
            if (remainingLength >= OptimizerConstants::GOOD_LEFTOVER_MIN &&
                remainingLength <= OptimizerConstants::GOOD_LEFTOVER_MAX) {
                break;
            }

            // Köztes tartomány (300–500) → próbáljunk megszabadulni tőle
            if (remainingLength >= OptimizerConstants::SELEJT_THRESHOLD &&
                remainingLength < OptimizerConstants::GOOD_LEFTOVER_MIN) {
                auto onePieceFit = OptimizerUtils::findSingleBestPiece(groupVec, remainingLength, kerf_mm);
                if (onePieceFit.has_value()) {
                    const Piece::PieceWithMaterial &piece = *onePieceFit;

                    int used = piece.info.length_mm +
                               OptimizerUtils::roundKerfLoss(1, kerf_mm);
                    int newRemaining = remainingLength - used;

                    auto cutStatus2 = cutSinglePieceBatch(piece, remainingLength, rod, machine,
                                        currentOpId, rodId, kerf_mm, groupVec);
                    // if(cutStatus2 == CutStatus::Overfill)
                    //      break;

                    if (newRemaining < OptimizerConstants::SELEJT_THRESHOLD) {
                        continue; // teljesen elfogyott → új rúd
                    } else {
                        break; // jó leftover → lezárjuk
                    }
                }
                // if (remainingLength > 0) {
                //     LeftoverStockEntry entry;
                //     entry.materialId = rod.materialId;
                //     entry.availableLength_mm = remainingLength;
                //     entry.used = false;
                //     entry.barcode = QString("UU2-%1").arg(QUuid::createUuid().toString().mid(1, 6));
                //     _localLeftovers.append(entry);
                // }
                break;
            }

            // Túl nagy leftover (> 800) → próbáljunk még egy darabot
            if (remainingLength > OptimizerConstants::GOOD_LEFTOVER_MAX) {
                auto onePieceFit = OptimizerUtils::findSingleBestPiece(groupVec, remainingLength, kerf_mm);
                if (onePieceFit.has_value()) {
                    cutSinglePieceBatch(*onePieceFit, remainingLength, rod, machine, currentOpId, rodId, kerf_mm, groupVec);
                    continue; // folytatjuk a rod-loopot
                }
                break; // ha nincs, lezárjukResultModel
            }


            // Ha maradt még anyag, de nem esett bele egyik stop feltételbe sem → leftoverként elmentjük
            if (remainingLength > 0) {
                LeftoverStockEntry entry;
                entry.materialId = rod.materialId;
                entry.availableLength_mm = remainingLength;
                entry.used = false;
                entry.barcode = IdentifierUtils::makeLeftoverId(SettingsManager::instance().nextMaterialCounter());
                entry.entryId = QUuid::createUuid();
                entry.parentBarcode = rod.barcode;
                entry.source = Cutting::Result::LeftoverSource::Optimization;
                entry.optimizationId = std::make_optional(currentOpId);

                leftoverRodMap.insert(entry.entryId, rod.rodId);




                // auto newRodId = IdentifierUtils::makeRodId(++rodCounter);
                // leftoverRodMap.insert(entry.entryId, newRodId);


                _usedLeftoverEntryIds.insert(entry.entryId);
                _localLeftovers.append(entry);

                zEvent(QString("📦 Új leftover létrehozva rod‑loop végén: %1 (%2 mm)")
                           .arg(entry.barcode).arg(entry.availableLength_mm));
            } else {
                zInfo(QString("ROD-LOOP END: rodId=%1 → no leftover (remainingLength=0)")
                          .arg(rod.rodId));
            }

            break; // nincs több értelmes darab → lezárjuk


        }// rod-loop vége
        rodSelected = false;   // ⛔ kötelező új rúd választása
        remainingLength = 0;   // ⛔ a rúd biztosan lezárult
        remainingLength2 = 0;
        //rod = SelectedRod();
    }
    // A lokális leftoverokat commitoljuk a globális készletbe
    // A lokális leftoverokat commitoljuk a globális készletbe
    for (const auto& entry : _localLeftovers) {
        _inventorySnapshot.reusableInventory.append(entry);
        // zEvent(QString("📦 Commit leftover: %1 (%2 mm)")
        //            .arg(entry.barcode).arg(entry.availableLength_mm));
    }
    _localLeftovers.clear();


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

OptimizerModel::CutStatus OptimizerModel::cutSinglePieceBatch(const Cutting::Piece::PieceWithMaterial& piece,
                                         int& remainingLength,
                                         const SelectedRod& rod,
                                         const CuttingMachine& machine,
                                         int currentOpId,
                                         int rodId,
                                         double kerf_mm,
                                         QVector<Cutting::Piece::PieceWithMaterial>& groupVec)
{
    int used = piece.info.length_mm + OptimizerUtils::roundKerfLoss(1, kerf_mm);

    zInfo(QString("CUT: rodId=%1, barcode=%2, totalLength=%3, remainingBefore=%4, used=%5")
              .arg(rod.rodId)
              .arg(rod.barcode)
              .arg(rod.length)
              .arg(remainingLength)
              .arg(used));

    // 🔒 Overfill-védelem
    if (used > remainingLength) {
        zError(QString("❌ Overfill detected in cutSinglePieceBatch: used=%1 > remaining=%2 (rodId=%3)")
                   .arg(used).arg(remainingLength).arg(rod.rodId));
        //remainingLength = 0;
        return CutStatus::Overfill;
    }

    int waste = OptimizerUtils::computeWasteInt(remainingLength, used);

    // 📦 CutPlan
    Cutting::Plan::CutPlan p;
    p.planNumber = planCounter++;   // 🔢 Globális sorszám kiosztása
    p.piecesWithMaterial = { piece };
    p.kerfTotal = OptimizerUtils::roundKerfLoss(1, kerf_mm);
    p.waste = remainingLength - used;  // tényleges maradék a vágás után;
    p.materialId = rod.materialId;
    p.rodId = rod.rodId;  // mindig a SelectedRod rodId-ja (öröklődik vagy új generált)
    p.source = rod.isReusable ? Cutting::Plan::Source::Reusable : Cutting::Plan::Source::Stock;
    p.planId = QUuid::createUuid();
    p.status = Cutting::Plan::Status::NotStarted;
    p.totalLength = rod.length;   // mindig a teljes rúd hossza;
    //p.totalLength = remainingLength + used;   // effektív hossz
    p.machineId   = machine.id;
    p.machineName = machine.name;
    p.kerfUsed_mm = kerf_mm;

    // ez a cutplan piecesWithMaterial-ből csinál szegmenseket, a maradék az waste
    p.generateSegments(kerf_mm, remainingLength);
    p.sourceBarcode = rod.barcode;   // MAT-xxx vagy RST-xxx
    p.optimizationId = currentOpId;

    p.parentBarcode = rod.barcode;

    // if (rod.isReusable) {
    //     p.parentBarcode = rod.barcode;
    // } else {
    //     p.parentBarcode = std::nullopt;
    // }

    p.parentPlanId  = std::nullopt; // később, ha láncolni akarod

    QString wasteBarcode = p.getWasteBarcode();
    p.leftoverBarcode = wasteBarcode;

    zEvent(OptimizerUtils::formatCutPlanEvent(p, machine));


    // ➕ ResultModel
    Cutting::Result::ResultModel result;
    result.cutPlanId = p.planId;
    result.materialId = rod.materialId;
    result.length = rod.length;
    result.cuts = { piece };
    result.waste = waste;
    result.source = rod.isReusable
                        ? Cutting::Result::ResultSource::FromReusable
                        : Cutting::Result::ResultSource::FromStock;
    result.optimizationId = rod.isReusable
                                ? std::nullopt
                                : std::make_optional(currentOpId);
    result.reusableBarcode = wasteBarcode;
    result.isFinalWaste = (remainingLength - used <= 0);
    result.parentBarcode = p.parentBarcode; // 🔗 auditlánc
    result.sourceBarcode = rod.barcode;     // 🔗 eredeti rúd


    // ♻️ Leftover visszarakása
    if (!result.isFinalWaste && result.waste > 0 && used <= remainingLength + used) {
        // ⚠️ FONTOS: a LeftoverStockEntry default CTOR már generál egyedi entryId-t.
        // Itt TILOS újra beállítani, mert a konstruktorban már létrejött a GUID.
        LeftoverStockEntry entry;

        entry.materialId = result.materialId;
        entry.availableLength_mm = result.waste;
        entry.used = false;
        entry.barcode = result.reusableBarcode;
        entry.parentBarcode = rod.barcode;
        // entry.parentBarcode = rod.isReusable
        //                           ? p.parentBarcode
        //                           : std::nullopt;
        entry.source = Result::LeftoverSource::Optimization;
        entry.optimizationId = std::make_optional(currentOpId);

        //zEvent(OptimizerUtils::formatLeftoverEvent(entry, rod.rodId));

        zInfo(QString("♻️ Assigned RST barcode=%1 for leftover entryId=%2 (rodId=%3)")
                  .arg(entry.barcode)
                  .arg(entry.entryId.toString())
                  .arg(rod.rodId));


         // 🔑 Regisztráljuk a leftover → rodId kapcsolatot
         leftoverRodMap.insert(entry.entryId, rod.rodId);
         zInfo(QString("MAP-INSERT: %1 → %2")
                          .arg(entry.entryId.toString())
                          .arg(rod.rodId));

         // Csak utána tesszük be a listába
         _localLeftovers.append(entry);

         // ⛔ A forrás leftover tiltása, nem az új waste-é
         if (rod.isReusable) {
             if (rod.entryId.has_value()) {
                 _usedLeftoverEntryIds.insert(rod.entryId.value());
                 zInfo(QString("TILTÁS: forrás leftover entryId=%1 (rodId=%2)")
                           .arg(rod.entryId->toString())
                           .arg(rod.rodId));
             } else {
                 zError(QString("❌ Inconsistent state: reusable rod without entryId (rodId=%1, barcode=%2)")
                            .arg(rod.rodId)
                            .arg(rod.barcode));
                 Q_ASSERT(false);// vagy return; hogy ne folytassa
             }
         }

         // 🔍 ParentBarcode log
         zInfo(QString("ParentBarcode set in ResultModel=%1, entry.parentBarcode=%2")
                   .arg(result.parentBarcode.value_or("∅"))
                   .arg(entry.parentBarcode.value_or("∅")));
    }

    _planned_leftovers.append(result);
    _result_plans.append(p);

    // Darab törlése
    groupVec.erase(std::remove_if(groupVec.begin(), groupVec.end(),
                                  [&](const auto& candidate){
                                      return candidate.info.pieceId == piece.info.pieceId;
                                  }), groupVec.end());

    remainingLength -= used;

    zInfo(QString("CUT-END: rodId=%1, remainingAfter=%2")
              .arg(rod.rodId)
              .arg(remainingLength));

    return CutStatus::Ok;


}

OptimizerModel::CutStatus OptimizerModel::cutComboBatch(const QVector<Cutting::Piece::PieceWithMaterial>& combo,
                                   int& remainingLength,
                                   const SelectedRod& rod,
                                   const CuttingMachine& machine,
                                   int currentOpId,
                                   int rodId,
                                   double kerf_mm,
                                   QVector<Cutting::Piece::PieceWithMaterial>& groupVec)
{

    int totalCut  = OptimizerUtils::sumLengths(combo);
    int kerfTotal = OptimizerUtils::roundKerfLoss(combo.size(), kerf_mm);
    int used      = totalCut + kerfTotal;

    zInfo(QString("CUT: rodId=%1, barcode=%2, totalLength=%3, remainingBefore=%4, used=%5")
              .arg(rod.rodId)
              .arg(rod.barcode)
              .arg(rod.length)
              .arg(remainingLength)
              .arg(used));


    // 🔒 Overfill-védelem
    if (used > remainingLength) {
        zError(QString("❌ Overfill detected in cutComboBatch: used=%1 > remaining=%2 (rodId=%3)")
                   .arg(used).arg(remainingLength).arg(rod.rodId));
        //remainingLength = 0;
        return CutStatus::Overfill; // a batch érvénytelen → nem vágunk
    }


    int waste     = OptimizerUtils::computeWasteInt(remainingLength, used);

    // 📦 CutPlan
    Cutting::Plan::CutPlan p;
    //p.rodNumber = rodId;
    p.planNumber = planCounter++;   // 🔢 Globális sorszám kiosztása
    p.piecesWithMaterial = combo;
    p.kerfTotal = kerfTotal;
    p.waste =  remainingLength - used;  // tényleges maradék a vágás után;
    p.materialId = rod.materialId;
    p.rodId = rod.rodId;  // mindig a SelectedRod rodId-ja
    p.source = rod.isReusable ? Cutting::Plan::Source::Reusable : Cutting::Plan::Source::Stock;
    p.planId = QUuid::createUuid();
    p.status = Cutting::Plan::Status::NotStarted;
    p.totalLength = rod.length;   // mindig a teljes rúd hossza;
    //p.totalLength = remainingLength + used;   // effektív hossz
    p.machineId   = machine.id;
    p.machineName = machine.name;
    p.kerfUsed_mm = kerf_mm;
    p.generateSegments(kerf_mm, remainingLength);
    p.sourceBarcode = rod.barcode;   // MAT-xxx vagy RST-xxx
    p.optimizationId = currentOpId;

    p.parentBarcode = rod.barcode;

    // if (rod.isReusable) {
    //     p.parentBarcode = rod.barcode;
    // } else {
    //     p.parentBarcode = std::nullopt;
    // }
    p.parentPlanId  = std::nullopt; // később, ha láncolni akarod

    QString wasteBarcode = p.getWasteBarcode();

    p.leftoverBarcode = wasteBarcode;

    zEvent(OptimizerUtils::formatCutPlanEvent(p, machine));

    // ➕ ResultModel
    Cutting::Result::ResultModel result;
    result.cutPlanId = p.planId;
    result.materialId = rod.materialId;
    result.length = rod.length;
    result.cuts = combo;
    result.waste = waste;
    result.source = rod.isReusable
                        ? Cutting::Result::ResultSource::FromReusable
                        : Cutting::Result::ResultSource::FromStock;
    result.optimizationId = rod.isReusable
                                ? std::nullopt
                                : std::make_optional(currentOpId);
    result.reusableBarcode = wasteBarcode;
    result.isFinalWaste = (remainingLength - used <= 0);
    result.parentBarcode = p.parentBarcode; // 🔗 auditlánc
    result.sourceBarcode = rod.barcode;     // 🔗 eredeti rúd

    if (!result.isFinalWaste && result.waste > 0 && used <= remainingLength + used) {
        // ⚠️ FONTOS: a LeftoverStockEntry default CTOR már generál egyedi entryId-t.
        // Itt TILOS újra beállítani, mert a konstruktorban már létrejött a GUID.
        LeftoverStockEntry entry;

        entry.materialId = result.materialId;
        entry.availableLength_mm = result.waste;
        entry.used = false;
        entry.barcode = result.reusableBarcode;
        entry.parentBarcode = rod.barcode;/*rod.isReusable
                                  ? p.parentBarcode
                                  : std::nullopt;*/
        entry.source = Result::LeftoverSource::Optimization;
        entry.optimizationId = std::make_optional(currentOpId);


        //zEvent(OptimizerUtils::formatLeftoverEvent(entry, rod.rodId));

        zInfo(QString("♻️ Assigned RST barcode=%1 for leftover entryId=%2 (rodId=%3)")
                  .arg(entry.barcode)
                  .arg(entry.entryId.toString())
                  .arg(rod.rodId));


        // 🔑 Regisztráljuk a leftover → rodId kapcsolatot
        leftoverRodMap.insert(entry.entryId, rod.rodId);
        zInfo(QString("MAP-INSERT: %1 → %2")
                  .arg(entry.entryId.toString())
                  .arg(rod.rodId));

        // Csak utána tesszük be a listába
        _localLeftovers.append(entry);

        // ⛔ A forrás leftover tiltása, nem az új waste-é
        if (rod.isReusable) {
            if (rod.entryId.has_value()) {
                _usedLeftoverEntryIds.insert(rod.entryId.value());
                zInfo(QString("TILTÁS: forrás leftover entryId=%1 (rodId=%2)")
                          .arg(rod.entryId->toString())
                          .arg(rod.rodId));
            } else {
                zError(QString("❌ Inconsistent state: reusable rod without entryId (rodId=%1, barcode=%2)")
                           .arg(rod.rodId)
                           .arg(rod.barcode));
                Q_ASSERT(false); //vagy return; hogy ne folytassa
            }
        }

        // 🔍 ParentBarcode log
        zInfo(QString("ParentBarcode set in ResultModel=%1, entry.parentBarcode=%2")
                  .arg(result.parentBarcode.value_or("∅"))
                  .arg(entry.parentBarcode.value_or("∅")));
    }

    _result_plans.append(p);
    _planned_leftovers.append(result);


    // Darabok törlése
    groupVec.erase(std::remove_if(groupVec.begin(), groupVec.end(),
                                  [&](const auto& candidate){
                                      return std::any_of(combo.begin(), combo.end(),
                                                         [&](const auto& used){
                                                             return candidate.info.pieceId == used.info.pieceId;
                                                         });
                                  }), groupVec.end());

    remainingLength -= used;

    zInfo(QString("CUT-END: rodId=%1, remainingAfter=%2")
              .arg(rod.rodId)
              .arg(remainingLength));

    return CutStatus::Ok;
}





/*
Sok darabot preferál	1000 vagy több
Kis hulladékot preferál	100–300
Kiegyensúlyozott	500–800
*/

// QVector<Cutting::Piece::PieceWithMaterial>
// OptimizerModel::findBestFit(const QVector<Cutting::Piece::PieceWithMaterial>& available,
//                             int lengthLimit,
//                             double kerf_mm) const {
//     QVector<Cutting::Piece::PieceWithMaterial> bestCombo;
//     int bestScore = std::numeric_limits<int>::min();
//     int n = available.size();
//     int totalCombos = 1 << n;

//     // ⚡ Quick path: ha minden darab belefér, nincs mit optimalizálni
//     int totalRelevant = OptimizerUtils::sumLengths(available);
//     int maxKerf = OptimizerUtils::roundKerfLoss(available.size(), kerf_mm);
//     if (totalRelevant + maxKerf <= lengthLimit) {
//         return available;
//     }

//     for (int mask = 1; mask < totalCombos; ++mask) {
//         QVector<Cutting::Piece::PieceWithMaterial> combo;
//         for (int i = 0; i < n; ++i) {
//             if (mask & (1 << i)) {
//                 combo.append(available[i]);
//             }
//         }
//         if (combo.isEmpty()) continue;

//         int totalCut = OptimizerUtils::sumLengths(combo);
//         if (totalCut > lengthLimit) continue; // early discard

//         int kerfTotal = OptimizerUtils::roundKerfLoss(combo.size(), kerf_mm);
//         int used = totalCut + kerfTotal;

//         if (used <= lengthLimit) {
//             int waste = lengthLimit - used;
//             int leftoverLength = lengthLimit - used;
//             int score = OptimizerUtils::calcScore(combo.size(), waste, leftoverLength);

//             if (score > bestScore) {
//                 bestScore = score;
//                 bestCombo = combo;
//             }
//         }
//     }

//     return bestCombo;
// }

QVector<Cutting::Piece::PieceWithMaterial>
OptimizerModel::findBestFit(const QVector<Cutting::Piece::PieceWithMaterial>& available,
                            int lengthLimit,
                            double kerf_mm) const
{
    QVector<Cutting::Piece::PieceWithMaterial> bestCombo;
    int n = available.size();

    // 0) Ha nincs darab
    if (n == 0)
        return {};

    // Gyors visszatérés: ha a teljes hossz + maximális kerf (darabszám × kerf) is belefér,
    // akkor biztosan belefér a valóságos kerf-modellel is (ahol az első darab kerf=0).

    int totalRelevant = OptimizerUtils::sumLengths(available);
    int maxKerf = OptimizerUtils::roundKerfLoss(n, kerf_mm);
    if (totalRelevant + maxKerf <= lengthLimit)
        return available;

    // 2) Ha n <= 20 → brute-force (tökéletes)
    if (n <= 20) {
        int bestScore = std::numeric_limits<int>::min();
        int totalCombos = 1 << n;

        for (int mask = 1; mask < totalCombos; ++mask) {
            QVector<Cutting::Piece::PieceWithMaterial> combo;
            for (int i = 0; i < n; ++i)
                if (mask & (1 << i))
                    combo.append(available[i]);

            int totalCut = OptimizerUtils::sumLengths(combo);
            if (totalCut > lengthLimit) continue;

            int kerfTotal = OptimizerUtils::roundKerfLoss(combo.size(), kerf_mm);
            int used = totalCut + kerfTotal;

            // 🔒 Overfill-védelem
            if (used > lengthLimit)
                continue;

            int waste = lengthLimit - used;
            int score = OptimizerUtils::calcScore(combo.size(), waste, waste);

            if (score > bestScore) {
                bestScore = score;
                bestCombo = combo;
            }
        }
        return bestCombo;
    }

    // 3) DP fallback (nagy n, de kezelhető hosszLimit)
    if (lengthLimit <= 10000) {
        int maxKerf = OptimizerUtils::roundKerfLoss(n, kerf_mm);
        int dpLimit = lengthLimit - maxKerf;
        if (dpLimit < 0) dpLimit = 0;

        struct Item { int len; Cutting::Piece::PieceWithMaterial piece; };
        QVector<Item> items;
        items.reserve(n);


        for (const auto& p : available)
            items.append({ p.info.length_mm, p });

        QVector<int> dp(dpLimit + 1, -1);
        QVector<int> parent(dpLimit + 1, -1);

        dp[0] = 0;

        for (int i = 0; i < n; ++i) {
            int len = items[i].len; // kerf nélkül
            for (int w = dpLimit; w >= len; --w) {
                if (dp[w - len] != -1 && dp[w - len] + len > dp[w]) {
                    dp[w] = dp[w - len] + len;
                    parent[w] = i;
                }
            }
        }

        int bestW = 0;
        for (int w = 1; w <= dpLimit; ++w)
            if (dp[w] > dp[bestW])
                bestW = w;

        QVector<Cutting::Piece::PieceWithMaterial> result;
        int w = bestW;
        while (w > 0 && parent[w] != -1) {
            int idx = parent[w];
            result.append(items[idx].piece);
            w -= items[idx].len;
        }

        // kerf utólagos számítása
        int kerfTotal = OptimizerUtils::roundKerfLoss(result.size(), kerf_mm);
        int used = OptimizerUtils::sumLengths(result) + kerfTotal;
        if (used <= lengthLimit)
            return result;
        // greedy fallback
    }

    // 4) Greedy fallback (nagyon nagy n vagy nagy lengthLimit)
    auto sorted = available;
    std::sort(sorted.begin(), sorted.end(),
              [](const auto& a, const auto& b){
                  return a.info.length_mm > b.info.length_mm;
              });

    QVector<Cutting::Piece::PieceWithMaterial> greedy;
    int used = 0;

    for (const auto& p : sorted) {
        // első darab: nincs vágás → 0 kerf
        // további darabok: minden új darabhoz 1 vágás → kerf_mm

        int kerf = greedy.isEmpty()
        ? 0
        : OptimizerUtils::roundKerfLoss(1, kerf_mm); // egységes kerf: első darab 0, továbbiak 1×kerf

        int candidateUsed = used + p.info.length_mm + kerf;

        if (candidateUsed <= lengthLimit) {
            greedy.append(p);
            used = candidateUsed;
        }
    }

    return greedy;
}



/*
♻️ A reusable rudak sorbarendezése garantálja, hogy előbb próbáljuk a kisebb, „kockáztathatóbb” rudakat
✂️ A pontszámítás továbbra is érvényes: preferáljuk a több darabot és a kisebb hulladékot
 */
std::optional<OptimizerModel::ReusableCandidate>
OptimizerModel::findBestReusableFit(const QVector<LeftoverStockEntry>& mergedView,
                                    int globalCount,
                                    const QVector<Cutting::Piece::PieceWithMaterial>& pieces,
                                    QUuid materialId,
                                    double kerf_mm) const {
    std::optional<ReusableCandidate> best;
    int bestScore = std::numeric_limits<int>::min();
    QSet<QUuid> groupIds = GroupUtils::groupMembers(materialId);

    // 🔎 releváns darabok kiszűrése
    QVector<Cutting::Piece::PieceWithMaterial> relevantPieces;
    for (const auto& p : pieces)
        if (groupIds.contains(p.materialId))
            relevantPieces.append(p);

    for (int i = 0; i < mergedView.size(); ++i) {
        const auto& stock = mergedView[i];
        if (stock.used) continue;
        if (!groupIds.contains(stock.materialId)) continue;
        if (_usedLeftoverEntryIds.contains(stock.entryId)) continue; // ezt a hullót már elhasználtuk

        // PRIORITÁS: egy darab, ami pontosan elfogyasztja
        const auto single = OptimizerUtils::findSingleExactFit(relevantPieces, stock.availableLength_mm, kerf_mm);
        if (single.has_value()) {
            int kerfTotal = OptimizerUtils::roundKerfLoss(1, kerf_mm);
            int used = single->info.length_mm + kerfTotal;
            int waste = OptimizerUtils::computeWasteInt(stock.availableLength_mm, used);
            if (waste == 0) {
                return ReusableCandidate{ i, stock, QVector<Cutting::Piece::PieceWithMaterial>{ *single }, waste };
            }
        }

        // Egyébként: keresd a legjobb részhalmazt
        auto combo = findBestFit(relevantPieces, stock.availableLength_mm, kerf_mm);
        zInfo(QString("findBestFit: %1 darab, bestCombo size=%2")
                   .arg(relevantPieces.size()).arg(combo.size()));

        if (combo.isEmpty()) continue;

        int totalCut = OptimizerUtils::sumLengths(combo);
        int kerfTotal = OptimizerUtils::roundKerfLoss(combo.size(), kerf_mm);
        int used = totalCut + kerfTotal;
        if (used > stock.availableLength_mm) continue; // early discard

        int waste = OptimizerUtils::computeWasteInt(stock.availableLength_mm, used);
        int leftoverLength = stock.availableLength_mm - used;


        int score = OptimizerUtils::calcScore(combo.size(), waste, leftoverLength);

        if (score > bestScore) {
            bestScore = score;
            ReusableCandidate cand;
            cand.indexInView = i;
            cand.stock = stock;
            cand.combo = combo;
            cand.waste = waste;
            cand.source = (i < globalCount)
                              ? ReusableCandidate::Source::GlobalSnapshot
                              : ReusableCandidate::Source::LocalPool;
            best = cand;
        }
    }
    return best;
}



void OptimizerModel::setCuttingRequests(const QVector<Cutting::Plan::Request>& list) {
    _requests = list;
}

} //end namespace Optimizer
} //end namespace Cutting
