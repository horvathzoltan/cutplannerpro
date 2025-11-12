#include "optimizermodel.h"
#include "common/eventlogger.h"
#include "common/logger.h"
#include "model/material/materialgroup_utils.h"
#include "model/machine/machineutils.h"
#include "service/cutting/optimizer/optimizerutils.h"
//#include "service/cutting/segment/segmentutils.h"
//#include "model/profilestock.h"
//#include <numeric>
#include <algorithm>
#include <QSet>

#include <model/registries/materialregistry.h>
#include <QDebug>
#include <common/identifierutils.h>
#include <common/settingsmanager.h>

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

    // 2. Optimalizációs ciklus
    while (anyPending()) {
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
        SelectedRod rod;

        int remainingLength = 0;

        // Összefésült nézet készítése
        auto merged = globalSnapshot;
        merged += _localLeftovers;

        // ♻️ Először próbáljunk reusable-t
        if (std::optional<ReusableCandidate> candidate =
            findBestReusableFit(merged, globalSnapshot.size(), groupVec,targetMaterialId, kerf_mm)) {
            const auto &best = *candidate;

            rod.materialId = best.stock.materialId;
            rod.length = best.stock.availableLength_mm;
            rod.isReusable = true;
            rod.barcode = best.stock.barcode;
            rod.entryId = best.stock.entryId;

            // 🔍 RodId hozzárendelés a map alapján, fallback + azonnali regisztráció
            if (leftoverRodMap.contains(best.stock.entryId)) {
                rod.rodId = leftoverRodMap.value(best.stock.entryId);
                zInfo(QString("MAP-LOOKUP: %1 → %2")
                          .arg(best.stock.entryId.toString())
                          .arg(rod.rodId));
            } else {
                rod.rodId = IdentifierUtils::makeRodId(++rodCounter);
                zWarning(QString("⚠️ Missing rodId mapping for leftover %1, new rodId=%2")
                          .arg(best.stock.entryId.toString())
                          .arg(rod.rodId));
                // 🔑 Azonnal regisztráljuk, hogy a lánc ne szakadjon meg
                leftoverRodMap.insert(best.stock.entryId, rod.rodId);
                zInfo(QString("MAP-INSERT (on select): %1 → %2")
                          .arg(best.stock.entryId.toString())
                          .arg(rod.rodId));
            }

            // ⛔ Már most jelöljük használtként
            _usedLeftoverEntryIds.insert(best.stock.entryId);
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
        } else {
            // 🧱 Ha nincs, akkor stockból - Stock vizsgálata — ANYAGCSOPORT ALAPÚ
            QSet<QUuid> groupIds = GroupUtils::groupMembers(
                targetMaterialId); // már használod máshol is
            for (auto &stock : _inventorySnapshot.profileInventory) {
                if (!groupIds.contains(stock.materialId))
                    continue; // ← csoporttagság
                if (stock.quantity <= 0)
                    continue;

                stock.quantity--;
                rod.materialId =
                    stock.materialId; // ← lehet MÁS, mint targetMaterialId, de
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


                remainingLength = rod.length;
                break;
            }
        }

        if (remainingLength == 0) { groupVec.removeFirst(); continue; }
        ++rodId; // új rúd

        // 2/d. Rod‑loop
        // while (true) {
        //     // 1. Keressük a legjobb kombót
        //     auto combo = findBestFit(groupVec, remainingLength, kerf_mm);
        //     if (combo.isEmpty()) break;

        //     // 2. Vágás a kombóval
        //     cutComboBatch(combo, remainingLength, rod, machine, currentOpId, rodId, kerf_mm, groupVec);


        //     // 3. Stop condition logika
        //     // Selejt alá esés → gyakorlatilag nullára fogyott
        //     if (remainingLength < OptimizerConstants::SELEJT_THRESHOLD) {
        //         if (remainingLength > 0) {
        //             LeftoverStockEntry entry;
        //             entry.materialId = rod.materialId;
        //             entry.availableLength_mm = remainingLength;
        //             entry.used = false;
        //             entry.barcode = QString("RST-%1").arg(QUuid::createUuid().toString().mid(1, 6));
        //             _localLeftovers.append(entry);
        //          //   _usedLeftoverEntryIds.insert(entry.entryId);

        //         }
        //         break;
        //     }

        //     // Jó leftover tartomány (500–800) → álljunk meg
        //     if (remainingLength >= OptimizerConstants::GOOD_LEFTOVER_MIN &&
        //         remainingLength <= OptimizerConstants::GOOD_LEFTOVER_MAX) {
        //         break;
        //     }

        //     // Köztes tartomány (300–500) → próbáljunk megszabadulni tőle
        //     if (remainingLength >= OptimizerConstants::SELEJT_THRESHOLD &&
        //         remainingLength < OptimizerConstants::GOOD_LEFTOVER_MIN) {

        //         auto onePieceFit = OptimizerUtils::findSingleBestPiece(groupVec, remainingLength, kerf_mm);
        //         if (onePieceFit.has_value()) {
        //             const auto& piece = *onePieceFit;
        //             int used = piece.info.length_mm + OptimizerUtils::roundKerfLoss(1, kerf_mm);
        //             int newRemaining = remainingLength - used;

        //             if (newRemaining < OptimizerConstants::SELEJT_THRESHOLD ||
        //                 (newRemaining >= OptimizerConstants::GOOD_LEFTOVER_MIN &&
        //                  newRemaining <= OptimizerConstants::GOOD_LEFTOVER_MAX)) {
        //                 cutSinglePieceBatch(piece, remainingLength, rod, machine, currentOpId, rodId, kerf_mm, groupVec);
        //                 if (newRemaining < OptimizerConstants::SELEJT_THRESHOLD) {
        //                     continue; // teljesen elfogyott → próbálhatjuk új rúddal
        //                 } else {
        //                     break; // jó leftover → lezárjuk
        //                 }
        //             }
        //         }

        //         // Ha nincs értelmes darab → lezárjuk
        //         if (remainingLength > 0) {
        //             LeftoverStockEntry entry;
        //             entry.materialId = rod.materialId;
        //             entry.availableLength_mm = remainingLength;
        //             entry.used = false;
        //             entry.barcode = QString("RST-%1").arg(QUuid::createUuid().toString().mid(1, 6));
        //             _localLeftovers.append(entry);
        //          //   _usedLeftoverEntryIds.insert(entry.entryId);

        //         }
        //         break;
        //     }

        // Ha nincs értelmes darab → leftoverként elmentjük
        // if (remainingLength > 0) {
        //     LeftoverStockEntry entry;
        //     entry.materialId = rod.materialId;
        //     entry.availableLength_mm = remainingLength;
        //     entry.used = false;
        //     entry.barcode = IdentifierUtils::makeLeftoverId(SettingsManager::instance().nextMaterialCounter());
        //     entry.entryId = QUuid::createUuid();
        //     entry.parentBarcode = rod.barcode;
        //     entry.source = Cutting::Result::LeftoverSource::Optimization;
        //     entry.optimizationId = std::make_optional(currentOpId);

        //     leftoverRodMap.insert(entry.entryId, rod.rodId);
        //     _usedLeftoverEntryIds.insert(entry.entryId);
        //     _localLeftovers.append(entry);

        //     zEvent(QString("📦 Új leftover létrehozva köztes tartományból: %1 (%2 mm)")
        //                .arg(entry.barcode).arg(entry.availableLength_mm));
        // }
        // break;

        //     // Túl nagy leftover (> 800) → próbáljunk még egy darabot
        //     if (remainingLength > OptimizerConstants::GOOD_LEFTOVER_MAX) {
        //         auto onePieceFit = OptimizerUtils::findSingleBestPiece(groupVec, remainingLength, kerf_mm);
        //         if (onePieceFit.has_value()) {
        //             cutSinglePieceBatch(*onePieceFit, remainingLength,
        //                                 rod, machine, currentOpId,
        //                                 rodId, kerf_mm, groupVec);
        //             continue; // folytatjuk a rod-loopot
        //         }
        //         break; // ha nincs, lezárjuk
        //     }

        //     // Egyébként → nincs értelmes további vágás
        //     break;
        // }

        // // 2/d. Rod‑loop
        // while (true) {
        //     // 1. Keressük a legjobb kombót
        //     auto combo = findBestFit(groupVec, remainingLength, kerf_mm);
        //     if (combo.isEmpty()) break;

        //     // 2. Vágás a kombóval
        //     cutComboBatch(combo, remainingLength, rod, machine, currentOpId, rodId, kerf_mm, groupVec);

        //     // 3. Stop condition logika – minden esetben próbáljunk még egy darabot
        //     auto onePieceFit = OptimizerUtils::findSingleBestPiece(groupVec, remainingLength, kerf_mm);
        //     if (onePieceFit.has_value()) {
        //         cutSinglePieceBatch(*onePieceFit, remainingLength, rod, machine, currentOpId, rodId, kerf_mm, groupVec);
        //         continue; // folytatjuk a rod-loopot
        //     }

        //     // Ha nincs értelmes darab, akkor keletkezhet leftover
        //     if (remainingLength > 0) {
        //         LeftoverStockEntry entry;
        //         entry.materialId = rod.materialId;
        //         entry.availableLength_mm = remainingLength;
        //         entry.used = false;
        //         entry.barcode = QString("RST-%1").arg(QUuid::createUuid().toString().mid(1, 6));
        //         // 🆔 Stabil entryId (ha reusable, örökölt; ha stock, frissen generált)
        //         if (rod.isReusable && rod.entryId.has_value()) {
        //             entry.entryId = rod.entryId.value();
        //         } else {
        //             entry.entryId = QUuid::createUuid();
        //         }
        //         // 🔗 Azonnali mapping regisztráció és használt jelölés (egy runtime-on belül)
        //         leftoverRodMap.insert(entry.entryId, rod.rodId);
        //         //_usedLeftoverEntryIds.insert(entry.entryId);
        //         _localLeftovers.append(entry);
        //         // ⛔ Ne tiltsuk ki – csak akkor, ha ténylegesen vágás történt belőle
        //     }

        //     break; // nincs több értelmes darab → lezárjuk
        // }

        // 2/d. Rod‑loop stop feltételekkel
        while (true) {
            auto combo = findBestFit(groupVec, remainingLength, kerf_mm);
            zInfo(QString("findBestFit: %1 darab, bestCombo size=%2")
                       .arg(groupVec.size()).arg(combo.size()));
            if (combo.isEmpty()) break;

            cutComboBatch(combo, remainingLength, rod, machine, currentOpId, rodId, kerf_mm, groupVec);

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

                    cutSinglePieceBatch(piece, remainingLength, rod, machine,
                                        currentOpId, rodId, kerf_mm, groupVec);

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
                break; // ha nincs, lezárjuk
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
                _usedLeftoverEntryIds.insert(entry.entryId);
                _localLeftovers.append(entry);

                zEvent(QString("📦 Új leftover létrehozva rod‑loop végén: %1 (%2 mm)")
                           .arg(entry.barcode).arg(entry.availableLength_mm));
            }

            break; // nincs több értelmes darab → lezárjuk
        }

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

void OptimizerModel::cutSinglePieceBatch(const Cutting::Piece::PieceWithMaterial& piece,
                                         int& remainingLength,
                                         const SelectedRod& rod,
                                         const CuttingMachine& machine,
                                         int currentOpId,
                                         int rodId,
                                         double kerf_mm,
                                         QVector<Cutting::Piece::PieceWithMaterial>& groupVec)
{
    int used = piece.info.length_mm + OptimizerUtils::roundKerfLoss(1, kerf_mm);
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
    if (!result.isFinalWaste && result.waste > 0) {
        LeftoverStockEntry entry;
        // 🆕 Mindig új entryId az új waste leftovernek
        //entry.entryId = QUuid::createUuid();

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
}

void OptimizerModel::cutComboBatch(const QVector<Cutting::Piece::PieceWithMaterial>& combo,
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

    if (!result.isFinalWaste && result.waste > 0) {
        LeftoverStockEntry entry;
        // 🆕 Mindig új entryId az új waste leftovernek
        //entry.entryId = QUuid::createUuid();

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
}





/*
Sok darabot preferál	1000 vagy több
Kis hulladékot preferál	100–300
Kiegyensúlyozott	500–800
*/

QVector<Cutting::Piece::PieceWithMaterial>
OptimizerModel::findBestFit(const QVector<Cutting::Piece::PieceWithMaterial>& available,
                            int lengthLimit,
                            double kerf_mm) const {
    QVector<Cutting::Piece::PieceWithMaterial> bestCombo;
    int bestScore = std::numeric_limits<int>::min();
    int n = available.size();
    int totalCombos = 1 << n;

    // ⚡ Quick path: ha minden darab belefér, nincs mit optimalizálni
    int totalRelevant = OptimizerUtils::sumLengths(available);
    int maxKerf = OptimizerUtils::roundKerfLoss(available.size(), kerf_mm);
    if (totalRelevant + maxKerf <= lengthLimit) {
        return available;
    }

    for (int mask = 1; mask < totalCombos; ++mask) {
        QVector<Cutting::Piece::PieceWithMaterial> combo;
        for (int i = 0; i < n; ++i) {
            if (mask & (1 << i)) {
                combo.append(available[i]);
            }
        }
        if (combo.isEmpty()) continue;

        int totalCut = OptimizerUtils::sumLengths(combo);
        if (totalCut > lengthLimit) continue; // early discard

        int kerfTotal = OptimizerUtils::roundKerfLoss(combo.size(), kerf_mm);
        int used = totalCut + kerfTotal;

        if (used <= lengthLimit) {
            int waste = lengthLimit - used;
            int leftoverLength = lengthLimit - used;
            int score = OptimizerUtils::calcScore(combo.size(), waste, leftoverLength);

            if (score > bestScore) {
                bestScore = score;
                bestCombo = combo;
            }
        }
    }

    return bestCombo;
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
                return ReusableCandidate{ i, stock, QVector{ *single }, waste };
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
