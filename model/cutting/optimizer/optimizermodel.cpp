#include "optimizermodel.h"
#include "common/grouputils.h"
#include "common/machineutils.h"
#include "common/optimizerutils.h"
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
    return _result_leftovers;
}

void OptimizerModel::optimize() {
    _result_plans.clear();
    _result_leftovers.clear();
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
        SelectedRod rod;

        // ♻️ 2/c. Reusable vizsgálata
        std::optional<ReusableCandidate> candidate =
            findBestReusableFit(reusableInventory, groupVec, targetMaterialId, kerf_mm);
        if (candidate.has_value()) {
            const auto& best = *candidate;
            rod.materialId = best.stock.materialId;
            rod.length     = best.stock.availableLength_mm;
            rod.isReusable = true;
            rod.barcode    = best.stock.reusableBarcode();
            piecesWithMaterial = best.combo;
            reusableInventory[best.indexInInventory].used = true;
        } else {
            // 🧱 2/d. Stock vizsgálata
            for (auto &stock : profileInventory) {
                if (stock.materialId == targetMaterialId && stock.quantity > 0) {
                    stock.quantity--;
                    rod.materialId = stock.materialId;
                    rod.length     = stock.master() ? stock.master()->stockLength_mm : 0;
                    rod.isReusable = false;
                    const auto& masterOpt = MaterialRegistry::instance().findById(rod.materialId);
                    rod.barcode = masterOpt ? masterOpt->barcode : "(nincs barcode)";
                    piecesWithMaterial = findBestFit(groupVec, rod.length, kerf_mm);
                    break;
                }
            }
        }

        if (piecesWithMaterial.isEmpty()) {
            groupVec.removeFirst();
            continue;
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

        _result_leftovers.append(result);

        // 📝 Audit log
        EventLogger::instance().zEvent(
            QString("🪚 CutPlan #%1 → gép=%2, kerf=%3 mm, waste=%4 mm")
                .arg(p.rodNumber)
                .arg(machine.name)
                .arg(kerf_mm)
                .arg(waste));
    }

    // 🧹 7. Reusable készlet takarítása
    reusableInventory.erase(
        std::remove_if(reusableInventory.begin(), reusableInventory.end(),
                       [](const LeftoverStockEntry& e){ return e.used; }),
        reusableInventory.end());
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
            int score = OptimizerUtils::calcScore(combo.size(), waste);
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
    QVector<LeftoverStockEntry> sorted = reusableInventory;
    std::sort(sorted.begin(), sorted.end(),
              [](const LeftoverStockEntry& a, const LeftoverStockEntry& b) {
                  return a.availableLength_mm < b.availableLength_mm;
              });

    for (int i = 0; i < sorted.size(); ++i) {
        const auto& stock = sorted[i];
        if (!groupIds.contains(stock.materialId)) continue;

        // ⚡ Quick path: ha minden darab belefér
        int totalRelevant = OptimizerUtils::sumLengths(relevantPieces);
        int maxKerf = OptimizerUtils::roundKerfLoss(relevantPieces.size(), kerf_mm);
        if (totalRelevant + maxKerf <= stock.availableLength_mm) {
            int waste = stock.availableLength_mm - (totalRelevant + maxKerf);
            return ReusableCandidate{ i, stock, relevantPieces, waste };
        }

        // Egyébként: keresd a legjobb részhalmazt
        auto combo = findBestFit(relevantPieces, stock.availableLength_mm, kerf_mm);
        if (combo.isEmpty()) continue;

        int totalCut = OptimizerUtils::sumLengths(combo);
        int kerfTotal = OptimizerUtils::roundKerfLoss(combo.size(), kerf_mm);
        int used = totalCut + kerfTotal;
        if (used > stock.availableLength_mm) continue; // early discard

        int waste = OptimizerUtils::computeWasteInt(stock.availableLength_mm, used);
        int score = OptimizerUtils::calcScore(combo.size(), waste);

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

void OptimizerModel::setStockInventory(const QVector<StockEntry>& list) {
    profileInventory = list;
}

void OptimizerModel::setReusableInventory(const QVector<LeftoverStockEntry>& reusable) {
    reusableInventory = reusable;
}



} //end namespace Optimizer
} //end namespace Cutting
