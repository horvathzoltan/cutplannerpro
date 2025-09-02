#include "optimizermodel.h"
#include "common/grouputils.h"
#include "service/cutting/segment/segmentutils.h"
//#include "model/profilestock.h"
#include <numeric>
#include <algorithm>
#include <QSet>

#include <model/registries/materialregistry.h>
#include <QDebug>

namespace Cutting {
namespace Optimizer {

OptimizerModel::OptimizerModel(QObject *parent) : QObject(parent) {}

void OptimizerModel::setKerf(int value) {
    kerf = value;
}

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

    // 🔧 Minden vágandó darabot kigyűjtünk darabonként — külön hosszal
  ;
    QVector<Cutting::Piece::PieceWithMaterial> pieces;
    for (const Cutting::Plan::Request &req : requests) {
        for (int i = 0; i < req.quantity; ++i) {
            Cutting::Piece::PieceInfo info;
            info.length_mm = req.requiredLength;
            info.ownerName = "a";//req.ownerName; // vagy "Ismeretlen", ha nincs
            info.externalReference = "b";//req.externalReference; // vagy ""
            info.isCompleted = false;

            pieces.append(Cutting::Piece::PieceWithMaterial(info, req.materialId));
        }
    }

    int rodId = 0;

    // 🔁 Addig keresünk rudat, amíg van vágandó darab
    while (!pieces.isEmpty()) {
        Cutting::Piece::PieceWithMaterial target = pieces.front();
        QSet<QUuid> groupIds = GroupUtils::groupMembers(target.materialId);

        // 🏷️ Az aktuálisan kiválasztott rúd metaadatai
        QUuid selectedMaterialId;
        int selectedLength = 0;
        //QVector<int> selectedCombo;
        QVector<Cutting::Piece::PieceWithMaterial> selectedCombo;

        bool found = false;
        bool isReusable = false;

        // ♻️ Megpróbálunk találni hullóból újravágható rudat
        std::optional<ReusableCandidate> candidate =
            findBestReusableFit(reusableInventory, pieces, target.materialId);
        if (candidate.has_value()) {
            const auto& best = *candidate;

            selectedMaterialId = best.stock.materialId;
            selectedLength     = best.stock.availableLength_mm;
            selectedCombo      = best.combo;

            reusableInventory.remove(best.indexInInventory); // ❌ már nincs darabszám, kihúzzuk
            isReusable = true;
            found = true;
        }

        // 🧱 Ha nem találtunk hullót, akkor keresünk a profilkészletben
        if (!found) {
            for (int i = 0; i < profileInventory.size(); ++i) {
                const auto& stock = profileInventory[i];
                if (groupIds.contains(stock.materialId) && stock.quantity > 0) {
                    profileInventory[i].quantity--; // készlet csökkentése

                    selectedMaterialId = stock.materialId;
                    selectedLength     = stock.master() ? stock.master()->stockLength_mm : 0;

                    QVector<Cutting::Piece::PieceWithMaterial> relevant;
                    for (const auto& p : pieces)
                        if (groupIds.contains(p.materialId))
                            relevant.append(p);

                    selectedCombo = findBestFit(relevant, selectedLength);
                    found = true;
                    break;
                }
            }
        }

        // 🚫 Ha egyik készletből sem tudunk vágni, eldobjuk az első darabot és folytatjuk
        if (!found || selectedCombo.isEmpty()) {
            pieces.removeOne(target);
            continue;
        }

        // ✂️ Kivágott darabokat eltávolítjuk a listából      

        for (const auto& used : selectedCombo) {
            for (int i = 0; i < pieces.size(); ++i) {
                if (pieces[i].info.length_mm == used.info.length_mm &&
                    groupIds.contains(pieces[i].materialId)) {
                    pieces.removeAt(i);
                    break;
                }
            }
        }


        // 📦 Vágási terv mentése
        //int totalCut = std::accumulate(selectedCombo.begin(), selectedCombo.end(), 0);
        int totalCut = std::accumulate(selectedCombo.begin(), selectedCombo.end(), 0,
                                       [](int sum, const Cutting::Piece::PieceWithMaterial& pwm) {
                                           return sum + pwm.info.length_mm;
                                       });

        int kerfTotal = (selectedCombo.size() ) * kerf; // vágási veszteség
        int used = totalCut + kerfTotal;
        int waste = selectedLength - used;

        QString barcode;
        if (isReusable && candidate.has_value()) {
            barcode = candidate->stock.reusableBarcode(); // 🧾 egyedi azonosító a reusable darabra
        } else {
            const auto& masterOpt = MaterialRegistry::instance().findById(selectedMaterialId);
            barcode = masterOpt ? masterOpt->barcode : "(nincs barcode)";
        }

        Cutting::Plan::CutPlan p;

        p.rodNumber = ++rodId;
        p.cuts = selectedCombo;                    // ✅ Minden darab metaadatával
        p.kerfTotal = kerfTotal;
        p.waste = waste;
        p.materialId = selectedMaterialId;
        p.rodId = barcode;                         // Ha reusable: barcode = reusableBarcode
        p.source = isReusable ? Cutting::Plan::Source::Reusable : Cutting::Plan::Source::Stock;
        p.planId = QUuid::createUuid();           // Egyedi azonosító
        p.status = Cutting::Plan::Status::NotStarted;
        p.totalLength = selectedLength;

        // ➕ Ha később `piecesInfo` visszakerül, az így tölthető:
        //for (const auto& pwm : selectedCombo)
        //    p.piecesInfo.append(pwm.info);

        // 📐 Szakaszgenerálás – vizuális modellezéshez
        p.generateSegments(kerf, selectedLength);

        _result_plans.append(p);

        // ➕ Maradék mentése, ha >300 mm — az újrafelhasználható
        //if (waste >= 300) {
            Cutting::Result::ResultModel result;
            result.cutPlanId = p.planId;
            result.materialId     = selectedMaterialId;
            result.length         = selectedLength;
            result.cuts           = selectedCombo;
            result.waste          = waste;
            //result.source         = usedReusable ? LeftoverSource::Manual : LeftoverSource::Optimization;
            result.source = isReusable ? Cutting::Result::ResultSource::FromReusable : Cutting::Result::ResultSource::FromStock;
            result.optimizationId = isReusable ? std::nullopt : std::make_optional(currentOpId);
            result.reusableBarcode = QString("RST-%1").arg(QUuid::createUuid().toString().mid(1, 6)); // 📛 egyedi azonosító
            result.isFinalWaste = Cutting::Segment::SegmentUtils::isTrailingWaste(result.waste, p.segments);

            _result_leftovers.append(result);
        //}
    }
}



/*
Sok darabot preferál	1000 vagy több
Kis hulladékot preferál	100–300
Kiegyensúlyozott	500–800
*/

QVector<Cutting::Piece::PieceWithMaterial> OptimizerModel::findBestFit(const QVector<Cutting::Piece::PieceWithMaterial>& available, int lengthLimit) const {
    QVector<Cutting::Piece::PieceWithMaterial> bestCombo;
    int bestScore = std::numeric_limits<int>::min();
    int n = available.size();
    int totalCombos = 1 << n;

    const int weight = 1000; // súly a darabszámhoz

    for (int mask = 1; mask < totalCombos; ++mask) {
        QVector<Cutting::Piece::PieceWithMaterial> combo;
        int total = 0;
        int count = 0;

        for (int i = 0; i < n; ++i) {
            if (mask & (1 << i)) {
                const auto& pwm = available[i];
                combo.append(pwm);
                total += pwm.info.length_mm;
                ++count;
            }
        }

        if (count == 0)
            continue;

        int kerfTotal = combo.size()*kerf;// (count - 1) * kerf;
        int used = total + kerfTotal;

        if (used <= lengthLimit) {
            int waste = lengthLimit - used;
            int score = count * weight - waste;

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
std::optional<OptimizerModel::ReusableCandidate> OptimizerModel::findBestReusableFit(
    const QVector<LeftoverStockEntry>& reusableInventory,
    const QVector<Cutting::Piece::PieceWithMaterial>& pieces,
    QUuid materialId
    ) const {
    std::optional<ReusableCandidate> best;
    int bestScore = std::numeric_limits<int>::min();
    QSet<QUuid> groupIds = GroupUtils::groupMembers(materialId);

    //QVector<int> pieceLengths;
    QVector<Cutting::Piece::PieceWithMaterial> relevantPieces;
    for (const auto& p : pieces)
        if (groupIds.contains(p.materialId))
            relevantPieces.append(p);

    QVector<LeftoverStockEntry> sorted = reusableInventory;
    std::sort(sorted.begin(), sorted.end(), [](const LeftoverStockEntry& a, const LeftoverStockEntry& b) {
        return a.availableLength_mm < b.availableLength_mm;
    });

    for (int i = 0; i < sorted.size(); ++i) {
        const auto& stock = sorted[i];
        if (!groupIds.contains(stock.materialId))
            continue;

        //QVector<int> combo = findBestFit(pieceLengths, stock.availableLength_mm);
        QVector<Cutting::Piece::PieceWithMaterial> combo = findBestFit(relevantPieces, stock.availableLength_mm);
        if (combo.isEmpty())
            continue;

        //int totalCut = std::accumulate(combo.begin(), combo.end(), 0);

        int totalCut = std::accumulate(combo.begin(), combo.end(), 0,
                                       [](int sum, const Cutting::Piece::PieceWithMaterial& pwm) {
                                           return sum + pwm.info.length_mm;
                                       });


        int kerfTotal = (combo.size() ) * kerf;
        int used = totalCut + kerfTotal;
        int waste = stock.availableLength_mm - used;

        int score = static_cast<int>(combo.size()) * 1000 - waste;

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
