#include "optimizermodel.h"
#include "model/material/materialgroup_utils.h"
#include "model/machine/machineutils.h"
#include "service/cutting/optimizer/optimizerutils.h"
#include "service/cutting/segment/segmentutils.h"
//#include "model/profilestock.h"
//#include <numeric>
#include <algorithm>
#include <QSet>

#include <model/registries/materialregistry.h>
#include <QDebug>

namespace Cutting {
namespace Optimizer {

OptimizerModel::OptimizerModel(QObject *parent) : QObject(parent) {}

// void OptimizerModel::setKerf(int value) {
//     kerf = value;
// }

QVector<Cutting::Plan::CutPlan>& OptimizerModel::getResult_PlansRef() {
    return _result_plans;
}

QVector<Cutting::Result::ResultModel> OptimizerModel::getResults_Leftovers() const {
    return _planned_leftovers;
}

void OptimizerModel::optimize(TargetHeuristic heuristic) {
    _result_plans.clear();
    _planned_leftovers.clear();
    int currentOpId = nextOptimizationId++;

    // 1. Darabok előkészítése anyag szerint
    QHash<QUuid, QVector<Cutting::Piece::PieceWithMaterial>> piecesByMaterial;
    for (const Cutting::Plan::Request &req : requests) {
        for (int i = 0; i < req.quantity; ++i) {
            Cutting::Piece::PieceInfo info;
            info.length_mm = req.requiredLength;
            info.ownerName = req.ownerName.isEmpty() ? "Ismeretlen" : req.ownerName;
            info.externalReference = req.externalReference;
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

        // ♻️ Először próbáljunk reusable-t
        if (auto candidate = findBestReusableFit(inventorySnapshot.reusableInventory, groupVec, targetMaterialId, kerf_mm)) {
            const auto& best = *candidate;
            rod.materialId = best.stock.materialId;
            rod.length     = best.stock.availableLength_mm;
            rod.isReusable = true;
            rod.barcode    = best.stock.reusableBarcode();
            inventorySnapshot.reusableInventory.removeAt(best.indexInInventory);
            remainingLength = rod.length;
        } else {
            // 🧱 Ha nincs, akkor stockból - Stock vizsgálata — ANYAGCSOPORT ALAPÚ
            QSet<QUuid> groupIds = GroupUtils::groupMembers(targetMaterialId); // már használod máshol is
            for (auto &stock : inventorySnapshot.profileInventory) {
                if (!groupIds.contains(stock.materialId)) continue; // ← csoporttagság
                if (stock.quantity <= 0) continue;

                stock.quantity--;
                rod.materialId = stock.materialId; // ← lehet MÁS, mint targetMaterialId, de csoporttag
                rod.length     = stock.master() ? stock.master()->stockLength_mm : 0;
                rod.isReusable = false;

                const auto& masterOpt = MaterialRegistry::instance().findById(rod.materialId);
                rod.barcode = masterOpt ? masterOpt->barcode : "(nincs barcode)";

                remainingLength = rod.length;
                break;
            }
        }

        if (remainingLength == 0) { groupVec.removeFirst(); continue; }
        ++rodId; // új rúd

        // 2/d. Rod‑loop
        while (true) {
                // 1. Keressük a legjobb kombót
            auto combo = findBestFit(groupVec, remainingLength, kerf_mm);
            if (combo.isEmpty()) break;

            // 2. Vágás a kombóval
            cutComboBatch(combo, remainingLength, rod, machine, currentOpId, rodId, kerf_mm, groupVec);

            // int totalCut  = OptimizerUtils::sumLengths(combo);
            // int kerfTotal = OptimizerUtils::roundKerfLoss(combo.size(), kerf_mm);
            // int used      = totalCut + kerfTotal;
            // if (used > remainingLength) break;

            // int waste = OptimizerUtils::computeWasteInt(remainingLength, used);

            // // 📦 CutPlan
            // Cutting::Plan::CutPlan p;
            // p.rodNumber = rodId;
            // p.piecesWithMaterial = combo;
            // p.kerfTotal = kerfTotal;
            // p.waste = waste;
            // p.materialId = rod.materialId;
            // p.rodId = rod.barcode;
            // p.source = rod.isReusable ? Cutting::Plan::Source::Reusable : Cutting::Plan::Source::Stock;
            // p.planId = QUuid::createUuid();
            // p.status = Cutting::Plan::Status::NotStarted;
            // p.totalLength = remainingLength;
            // p.machineId   = machine.id;
            // p.machineName = machine.name;
            // p.kerfUsed_mm = kerf_mm;
            // p.generateSegments(static_cast<int>(std::lround(kerf_mm)), remainingLength);

            // // CutPlan bővítés (ha van rá mező, vagy adj hozzá)
            // p.parentBarcode = rod.isReusable ? rod.barcode /* az RST */ : std::optional<QString>{};
            // p.parentPlanId  = std::optional<QUuid>{}; // ha az eredeti plan-ből jönne, ide lehet kötni

            // _result_plans.append(p);

            // // ➕ ResultModel közvetlenül itt
            // Cutting::Result::ResultModel result;
            // result.cutPlanId = p.planId;
            // result.materialId = rod.materialId;
            // result.length = rod.length;
            // result.cuts = combo;
            // result.waste = waste;
            // result.source = rod.isReusable ? Cutting::Result::ResultSource::FromReusable
            //                                : Cutting::Result::ResultSource::FromStock;
            // result.optimizationId = rod.isReusable ? std::nullopt : std::make_optional(currentOpId);
            // result.reusableBarcode = QString("RST-%1").arg(QUuid::createUuid().toString().mid(1, 6));
            // result.isFinalWaste = (remainingLength - used <= 0);

            // // ResultModel bővítés
            // result.parentBarcode = p.parentBarcode;
            // result.sourceBarcode = rod.barcode; // explicit: miből vágtunk ténylegesen

            // _planned_leftovers.append(result);

            // // ♻️ Leftover visszarakása
            // if (!result.isFinalWaste && result.waste > 0) {
            //     LeftoverStockEntry entry;
            //     entry.materialId = result.materialId;
            //     entry.availableLength_mm = result.waste;
            //     entry.used = false;
            //     entry.barcode = result.reusableBarcode;
            //     inventorySnapshot.reusableInventory.append(entry);
            // }

            // // Audit log
            // EventLogger::instance().zEvent(
            //     QString("🪚 CutPlan #%1 → gép=%2, kerf=%3 mm, waste=%4 mm")
            //         .arg(p.rodNumber)
            //         .arg(machine.name)
            //         .arg(kerf_mm)
            //         .arg(waste));

            // // Darabok törlése
            // groupVec.erase(std::remove_if(groupVec.begin(), groupVec.end(),
            //                               [&](const auto& candidate){
            //                                   return std::any_of(combo.begin(), combo.end(),
            //                                                      [&](const auto& used){
            //                                                          return candidate.info.pieceId == used.info.pieceId;
            //                                                      });
            //                               }), groupVec.end());

            // remainingLength -= used;

            // // Stop condition
            // int minPieceLength = std::numeric_limits<int>::max();
            // for (const auto& piece : groupVec)
            //     minPieceLength = std::min(minPieceLength, piece.info.length_mm);
            // if (minPieceLength == std::numeric_limits<int>::max()) minPieceLength = 0;

            // if (remainingLength < minPieceLength + kerf_mm) {
            //     if (remainingLength > 0) {
            //         LeftoverStockEntry entry;
            //         entry.materialId = rod.materialId;
            //         entry.availableLength_mm = remainingLength;
            //         entry.used = false;
            //         entry.barcode = QString("RST-%1").arg(QUuid::createUuid().toString().mid(1, 6));
            //         inventorySnapshot.reusableInventory.append(entry);
            //     }
            //     break;
            // }

            // if (combo.size() > 1) {
            //     auto onePieceFit = OptimizerUtils::findSingleBestPiece(groupVec, remainingLength, kerf_mm);
            //     if (onePieceFit.has_value()) {
            //                 cutSinglePieceBatch(*onePieceFit, remainingLength, rod, machine, currentOpId, rodId, kerf_mm, groupVec);
            //         // const auto& piece = *onePieceFit;
            //         // int used = piece.info.length_mm + OptimizerUtils::roundKerfLoss(1, kerf_mm);
            //         // int waste = OptimizerUtils::computeWasteInt(remainingLength, used);

            //         // // CutPlan létrehozása csak ezzel az egy darabbal
            //         // Cutting::Plan::CutPlan p;
            //         // p.rodNumber = rodId;
            //         // p.piecesWithMaterial = { piece };
            //         // p.kerfTotal = OptimizerUtils::roundKerfLoss(1, kerf_mm);
            //         // p.waste = waste;
            //         // p.materialId = rod.materialId;
            //         // p.rodId = rod.barcode;
            //         // p.source = rod.isReusable ? Cutting::Plan::Source::Reusable : Cutting::Plan::Source::Stock;
            //         // p.planId = QUuid::createUuid();
            //         // p.status = Cutting::Plan::Status::NotStarted;
            //         // p.totalLength = remainingLength;
            //         // p.machineId   = machine.id;
            //         // p.machineName = machine.name;
            //         // p.kerfUsed_mm = kerf_mm;
            //         // p.generateSegments(static_cast<int>(std::lround(kerf_mm)), remainingLength);

            //         // _result_plans.append(p);

            //         // // ResultModel is
            //         // Cutting::Result::ResultModel result;
            //         // result.cutPlanId = p.planId;
            //         // result.materialId = rod.materialId;
            //         // result.length = rod.length;
            //         // result.cuts = { piece };
            //         // result.waste = waste;
            //         // result.source = rod.isReusable ? Cutting::Result::ResultSource::FromReusable
            //         //                                : Cutting::Result::ResultSource::FromStock;
            //         // result.optimizationId = rod.isReusable ? std::nullopt : std::make_optional(currentOpId);
            //         // result.reusableBarcode = QString("RST-%1").arg(QUuid::createUuid().toString().mid(1, 6));
            //         // result.isFinalWaste = (remainingLength - used <= 0);
            //         // _planned_leftovers.append(result);

            //         // // Darab törlése
            //         // groupVec.erase(std::remove_if(groupVec.begin(), groupVec.end(),
            //         //                               [&](const auto& candidate){
            //         //                                   return candidate.info.pieceId == piece.info.pieceId;
            //         //                               }), groupVec.end());

            //         // remainingLength -= used;
            //     }
            // }


            // 3. Stop condition logika

            // Selejt alá esés → gyakorlatilag nullára fogyott
            if (remainingLength < OptimizerConstants::SELEJT_THRESHOLD) {
                if (remainingLength > 0) {
                    LeftoverStockEntry entry;
                    entry.materialId = rod.materialId;
                    entry.availableLength_mm = remainingLength;
                    entry.used = false;
                    entry.barcode = QString("RST-%1").arg(QUuid::createUuid().toString().mid(1, 6));
                    inventorySnapshot.reusableInventory.append(entry);
                }
                break;
            }

            // Jó leftover tartomány (500–800) → álljunk meg
            if (remainingLength >= OptimizerConstants::GOOD_LEFTOVER_MIN &&
                remainingLength <= OptimizerConstants::GOOD_LEFTOVER_MAX) {
                break;
            }

            // Köztes tartomány (300–500) → próbáljunk megszabadulni tőle
            if (remainingLength >= OptimizerConstants::SELEJT_THRESHOLD &&
                remainingLength < OptimizerConstants::GOOD_LEFTOVER_MIN) {

                auto onePieceFit = OptimizerUtils::findSingleBestPiece(groupVec, remainingLength, kerf_mm);
                if (onePieceFit.has_value()) {
                    const auto& piece = *onePieceFit;
                    int used = piece.info.length_mm + OptimizerUtils::roundKerfLoss(1, kerf_mm);
                    int newRemaining = remainingLength - used;

                    if (newRemaining < OptimizerConstants::SELEJT_THRESHOLD ||
                        (newRemaining >= OptimizerConstants::GOOD_LEFTOVER_MIN &&
                         newRemaining <= OptimizerConstants::GOOD_LEFTOVER_MAX)) {
                        cutSinglePieceBatch(piece, remainingLength, rod, machine, currentOpId, rodId, kerf_mm, groupVec);
                        if (newRemaining < OptimizerConstants::SELEJT_THRESHOLD) {
                            continue; // teljesen elfogyott → próbálhatjuk új rúddal
                        } else {
                            break; // jó leftover → lezárjuk
                        }
                    }
                }

                // Ha nincs értelmes darab → lezárjuk
                if (remainingLength > 0) {
                    LeftoverStockEntry entry;
                    entry.materialId = rod.materialId;
                    entry.availableLength_mm = remainingLength;
                    entry.used = false;
                    entry.barcode = QString("RST-%1").arg(QUuid::createUuid().toString().mid(1, 6));
                    inventorySnapshot.reusableInventory.append(entry);
                }
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

            // Egyébként → nincs értelmes további vágás
            break;
        }
    }
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
    p.rodNumber = rodId;
    p.planNumber = ++planCounter;   // 🔢 Globális sorszám kiosztása
    p.piecesWithMaterial = { piece };
    p.kerfTotal = OptimizerUtils::roundKerfLoss(1, kerf_mm);
    p.waste = waste;
    p.materialId = rod.materialId;
    p.rodId = rod.barcode;
    p.source = rod.isReusable ? Cutting::Plan::Source::Reusable : Cutting::Plan::Source::Stock;
    p.planId = QUuid::createUuid();
    p.status = Cutting::Plan::Status::NotStarted;
    p.totalLength = remainingLength;
    p.machineId   = machine.id;
    p.machineName = machine.name;
    p.kerfUsed_mm = kerf_mm;
    p.generateSegments(static_cast<int>(std::lround(kerf_mm)), remainingLength);

    p.parentBarcode = rod.isReusable ? rod.barcode : std::optional<QString>{};
    p.parentPlanId  = std::nullopt; // később, ha láncolni akarod

    _result_plans.append(p);

    // ➕ ResultModel
    Cutting::Result::ResultModel result;
    result.cutPlanId = p.planId;
    result.materialId = rod.materialId;
    result.length = rod.length;
    result.cuts = { piece };
    result.waste = waste;
    result.source = rod.isReusable ? Cutting::Result::ResultSource::FromReusable
                                   : Cutting::Result::ResultSource::FromStock;
    result.optimizationId = rod.isReusable ? std::nullopt : std::make_optional(currentOpId);
    result.reusableBarcode = QString("RST-%1").arg(QUuid::createUuid().toString().mid(1, 6));
    result.isFinalWaste = (remainingLength - used <= 0);

    result.parentBarcode = p.parentBarcode;
    result.sourceBarcode = rod.barcode;

    _planned_leftovers.append(result);

    // ♻️ Leftover visszarakása
    if (!result.isFinalWaste && result.waste > 0) {
        LeftoverStockEntry entry;
        entry.materialId = result.materialId;
        entry.availableLength_mm = result.waste;
        entry.used = false;
        entry.barcode = result.reusableBarcode;
        inventorySnapshot.reusableInventory.append(entry);
    }

    // Audit log

    zEvent(
        QString("🪚 CutPlan #%1 (single, rod=%2) → gép=%3, kerf=%4 mm, waste=%5 mm")
            .arg(p.planNumber)   // 🔢 Globális sorszám
            .arg(p.rodNumber)    // Rúd sorszám
            .arg(machine.name)
            .arg(kerf_mm)
            .arg(waste));

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
    p.rodNumber = rodId;
    p.planNumber = ++planCounter;   // 🔢 Globális sorszám kiosztása
    p.piecesWithMaterial = combo;
    p.kerfTotal = kerfTotal;
    p.waste = waste;
    p.materialId = rod.materialId;
    p.rodId = rod.barcode;
    p.source = rod.isReusable ? Cutting::Plan::Source::Reusable : Cutting::Plan::Source::Stock;
    p.planId = QUuid::createUuid();
    p.status = Cutting::Plan::Status::NotStarted;
    p.totalLength = remainingLength;
    p.machineId   = machine.id;
    p.machineName = machine.name;
    p.kerfUsed_mm = kerf_mm;
    p.generateSegments(static_cast<int>(std::lround(kerf_mm)), remainingLength);

    p.parentBarcode = rod.isReusable ? rod.barcode : std::optional<QString>{};
    p.parentPlanId  = std::nullopt; // később, ha láncolni akarod

    _result_plans.append(p);

    // ➕ ResultModel
    Cutting::Result::ResultModel result;
    result.cutPlanId = p.planId;
    result.materialId = rod.materialId;
    result.length = rod.length;
    result.cuts = combo;
    result.waste = waste;
    result.source = rod.isReusable ? Cutting::Result::ResultSource::FromReusable
                                   : Cutting::Result::ResultSource::FromStock;
    result.optimizationId = rod.isReusable ? std::nullopt : std::make_optional(currentOpId);
    result.reusableBarcode = QString("RST-%1").arg(QUuid::createUuid().toString().mid(1, 6));
    result.isFinalWaste = (remainingLength - used <= 0);

    result.parentBarcode = p.parentBarcode;
    result.sourceBarcode = rod.barcode;

    _planned_leftovers.append(result);

    if (!result.isFinalWaste && result.waste > 0) {
        LeftoverStockEntry entry;
        entry.materialId = result.materialId;
        entry.availableLength_mm = result.waste;
        entry.used = false;
        entry.barcode = result.reusableBarcode;
        inventorySnapshot.reusableInventory.append(entry);
    }

    zEvent(
        QString("🪚 CutPlan #%1 (single, rod=%2) → gép=%3, kerf=%4 mm, waste=%5 mm")
            .arg(p.planNumber)   // 🔢 Globális sorszám
            .arg(p.rodNumber)    // Rúd sorszám
            .arg(machine.name)
            .arg(kerf_mm)
            .arg(waste));

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


void OptimizerModel::optimize_old(TargetHeuristic heuristic) {
    _result_plans.clear();
    _planned_leftovers.clear();
    int currentOpId = nextOptimizationId++;

    // 🔧 1. Darabok előkészítése — anyag szerint csoportosítva
    QHash<QUuid, QVector<Cutting::Piece::PieceWithMaterial>> piecesByMaterial;
    for (const Cutting::Plan::Request &req : requests) {
        for (int i = 0; i < req.quantity; ++i) {
            Cutting::Piece::PieceInfo info;
            info.length_mm = req.requiredLength;
            info.ownerName = req.ownerName.isEmpty() ? "Ismeretlen" : req.ownerName;
            info.externalReference = req.externalReference;
            info.isCompleted = false;
            piecesByMaterial[req.materialId].append(
                Cutting::Piece::PieceWithMaterial(info, req.materialId));
        }
    }

    auto anyPending = [&]() -> bool {
        for (auto it = piecesByMaterial.begin(); it != piecesByMaterial.end(); ++it)
            if (!it.value().isEmpty()) return true;
        return false;
    };

    int rodId = 0;

    // 🔁 2. Optimalizációs ciklus
    while (anyPending()) {
        // 🎯 2/a. Cél anyagcsoport kiválasztása (heurisztika: legnagyobb összhossz)
        // 🎯 2/a. Cél anyagcsoport kiválasztása a beállított heurisztika alapján
        QUuid targetMaterialId;
        int bestMetric = -1;

        for (auto it = piecesByMaterial.begin(); it != piecesByMaterial.end(); ++it) {
            int metric = 0;
            switch (heuristic) {
            case TargetHeuristic::ByCount:
                metric = it.value().size();
                break;
            case TargetHeuristic::ByTotalLength:
                metric = OptimizerUtils::sumLengths(it.value());
                break;
            }

            if (metric > bestMetric) {
                bestMetric = metric;
                targetMaterialId = it.key();
            }
        }

        auto &groupVec = piecesByMaterial[targetMaterialId];
        if (groupVec.isEmpty()) continue;

        // ⚙️ 2/b. Gép kiválasztása
        auto machineOpt = MachineUtils::pickMachineForMaterial(targetMaterialId);
        if (!machineOpt) {
            groupVec.removeFirst();
            continue;
        }
        const CuttingMachine machine = *machineOpt;
        const double kerf_mm = machine.kerf_mm;

        QVector<Cutting::Piece::PieceWithMaterial> piecesWithMaterial;

        // ♻️ 2/c. Reusable vizsgálata
        SelectedRod rod;
        int remainingLength = 0;

        // ♻️ 2/c. Reusable vizsgálata
        std::optional<ReusableCandidate> candidate =
            findBestReusableFit(inventorySnapshot.reusableInventory, groupVec, targetMaterialId, kerf_mm);
        if (candidate.has_value()) {
            const auto& best = *candidate;
            rod.materialId = best.stock.materialId;
            rod.length     = best.stock.availableLength_mm;
            rod.isReusable = true;
            rod.barcode    = best.stock.reusableBarcode();
            inventorySnapshot.reusableInventory.removeAt(best.indexInInventory);
            remainingLength = rod.length;
        } else {
            // 🧱 2/d. Stock vizsgálata
            for (auto &stock : inventorySnapshot.profileInventory) {
                if (stock.materialId == targetMaterialId && stock.quantity > 0) {
                    stock.quantity--;
                    rod.materialId = stock.materialId;
                    rod.length     = stock.master() ? stock.master()->stockLength_mm : 0;
                    rod.isReusable = false;
                    const auto& masterOpt = MaterialRegistry::instance().findById(rod.materialId);
                    rod.barcode = masterOpt ? masterOpt->barcode : "(nincs barcode)";
                    remainingLength = rod.length;
                    break;
                }
            }
        }

        // 🔁 ÚJ belső rod‑loop
        while (true) {
            auto combo = findBestFit(groupVec, remainingLength, kerf_mm);
            if (combo.isEmpty()) break;

            int totalCut  = OptimizerUtils::sumLengths(combo);
            int kerfTotal = OptimizerUtils::roundKerfLoss(combo.size(), kerf_mm);
            int used      = totalCut + kerfTotal;
            if (used > remainingLength) break;

            int waste = OptimizerUtils::computeWasteInt(remainingLength, used);

            // 📦 CutPlan létrehozása
            Cutting::Plan::CutPlan p;
            p.rodNumber = ++rodId;
            p.piecesWithMaterial = combo;
            p.kerfTotal = kerfTotal;
            p.waste = waste;
            p.materialId = rod.materialId;
            p.rodId = rod.barcode;
            p.source = rod.isReusable ? Cutting::Plan::Source::Reusable : Cutting::Plan::Source::Stock;
            p.planId = QUuid::createUuid();
            p.status = Cutting::Plan::Status::NotStarted;
            p.totalLength = remainingLength;
            _result_plans.append(p);

            // ✂️ kivágott darabok törlése
            groupVec.erase(std::remove_if(groupVec.begin(), groupVec.end(),
                                          [&](const auto& candidate){
                                              return std::any_of(combo.begin(), combo.end(),
                                                                 [&](const auto& used){
                                                                     return candidate.info.pieceId == used.info.pieceId;
                                                                 });
                                          }), groupVec.end());

            remainingLength -= used;

            int minPieceLength = std::numeric_limits<int>::max();
            for (const auto& piece : groupVec) {
                minPieceLength = std::min(minPieceLength, piece.info.length_mm);
            }
            if (minPieceLength == std::numeric_limits<int>::max()) {
                minPieceLength = 0; // ha üres
            }

            // stop condition: ha túl kicsi vagy "jó leftover"
            if (remainingLength < minPieceLength + kerf_mm) {
                if (remainingLength > 0) {
                    LeftoverStockEntry entry;
                    entry.materialId = rod.materialId;
                    entry.availableLength_mm = remainingLength;
                    entry.used = false;
                    entry.barcode = QString("RST-%1").arg(QUuid::createUuid().toString().mid(1, 6));
                    inventorySnapshot.reusableInventory.append(entry);
                }
                break;
            }
        }

        // ✂️ 4. Kivágott darabok eltávolítása UUID alapján
        groupVec.erase(std::remove_if(groupVec.begin(), groupVec.end(),
                                      [&](const auto& candidate){
                                          return std::any_of(piecesWithMaterial.begin(), piecesWithMaterial.end(),
                                                             [&](const auto& used){
                                                                 return candidate.info.pieceId == used.info.pieceId;
                                                             });
                                      }), groupVec.end());

        // 📦 5. Vágási terv létrehozása
        int totalCut = OptimizerUtils::sumLengths(piecesWithMaterial);
        int kerfTotal = OptimizerUtils::roundKerfLoss(static_cast<int>(piecesWithMaterial.size()), kerf_mm);
        int used = totalCut + kerfTotal;
        int waste = OptimizerUtils::computeWasteInt(rod.length, used);

        Cutting::Plan::CutPlan p;
        p.rodNumber = ++rodId;
        p.piecesWithMaterial = piecesWithMaterial;
        p.kerfTotal = kerfTotal;
        p.waste = waste;
        p.materialId = rod.materialId;
        p.rodId = rod.barcode;
        p.source = rod.isReusable ? Cutting::Plan::Source::Reusable : Cutting::Plan::Source::Stock;
        p.planId = QUuid::createUuid();
        p.status = Cutting::Plan::Status::NotStarted;
        p.totalLength = rod.length;

        // ⚙️ Gépadatok
        p.machineId   = machine.id;
        p.machineName = machine.name;
        p.kerfUsed_mm = kerf_mm;

        // 📐 Szakaszgenerálás
        p.generateSegments(static_cast<int>(std::lround(kerf_mm)), rod.length);

        _result_plans.append(p);

        // ➕ 6. Hulló mentése
        Cutting::Result::ResultModel result;
        result.cutPlanId = p.planId;
        result.materialId = rod.materialId;
        result.length = rod.length;
        result.cuts = piecesWithMaterial;
        result.waste = waste;
        result.source = rod.isReusable ? Cutting::Result::ResultSource::FromReusable
                                       : Cutting::Result::ResultSource::FromStock;
        result.optimizationId = rod.isReusable ? std::nullopt : std::make_optional(currentOpId);
        result.reusableBarcode = QString("RST-%1").arg(QUuid::createUuid().toString().mid(1, 6));
        result.isFinalWaste = Cutting::Segment::SegmentUtils::isTrailingWaste(result.waste, p.segments);

        _planned_leftovers.append(result);

        // ♻️ Friss leftover visszacsatolása a készletbe
        if (!result.isFinalWaste && result.waste > 0) {
            LeftoverStockEntry entry;
            entry.materialId = result.materialId;
            entry.availableLength_mm = result.waste;
            entry.used = false;
            entry.barcode = result.reusableBarcode;

            inventorySnapshot.reusableInventory.append(entry);

            EventLogger::instance().zEvent(
                QString("♻️ Leftover visszarakva: %1 mm, material=%2, barcode=%3")
                    .arg(result.waste)
                    .arg(result.materialId.toString())
                    .arg(result.reusableBarcode));
        }


        // 📝 Audit log
        EventLogger::instance().zEvent(
            QString("🪚 CutPlan #%1 → gép=%2, kerf=%3 mm, waste=%4 mm")
                .arg(p.rodNumber)
                .arg(machine.name)
                .arg(kerf_mm)
                .arg(waste));
    }

    // 🧹 7. Reusable készlet takarítása
    // reusableInventory.erase(
    //     std::remove_if(reusableInventory.begin(), reusableInventory.end(),
    //                    [](const LeftoverStockEntry& e){ return e.used; }),
    //     reusableInventory.end());
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
OptimizerModel::findBestReusableFit(const QVector<LeftoverStockEntry>& reusableInventory,
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

    // Hullók rendezése hossz szerint
    // QVector<LeftoverStockEntry> sorted = reusableInventory;
    // std::sort(sorted.begin(), sorted.end(),
    //           [](const LeftoverStockEntry& a, const LeftoverStockEntry& b) {
    //               return a.availableLength_mm < b.availableLength_mm;
    //           });

    for (int i = 0; i < reusableInventory.size(); ++i) {
        const auto& stock = reusableInventory[i];
        if (stock.used) continue;
        if (!groupIds.contains(stock.materialId)) continue;

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
        if (combo.isEmpty()) continue;

        int totalCut = OptimizerUtils::sumLengths(combo);
        int kerfTotal = OptimizerUtils::roundKerfLoss(combo.size(), kerf_mm);
        int used = totalCut + kerfTotal;
        if (used > stock.availableLength_mm) continue; // early discard

        int waste = OptimizerUtils::computeWasteInt(stock.availableLength_mm, used);
        int leftoverLength = stock.availableLength_mm - used;
        int score = OptimizerUtils::calcScore(combo.size(), waste, leftoverLength);

        if (score > bestScore) {
            best = ReusableCandidate{ i, stock, combo, waste };
            bestScore = score;
        }
    }
    return best;
}



void OptimizerModel::setCuttingRequests(const QVector<Cutting::Plan::Request>& list) {
    requests = list;
}

} //end namespace Optimizer
} //end namespace Cutting
