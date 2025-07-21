#include "CuttingOptimizerModel.h"
//#include "model/profilestock.h"
#include <numeric>
#include <algorithm>
#include <QSet>

CuttingOptimizerModel::CuttingOptimizerModel(QObject *parent) : QObject(parent) {}

void CuttingOptimizerModel::clearRequests() {
    requests.clear();
    //allPieces.clear();
    plans.clear();
    leftoverResults.clear();
}

void CuttingOptimizerModel::addRequest(const CuttingRequest& req) {
    requests.append(req);
}

void CuttingOptimizerModel::setKerf(int value) {
    kerf = value;
}

QVector<CutPlan> CuttingOptimizerModel::getPlans() const {
    return plans;
}

QVector<CutResult> CuttingOptimizerModel::getLeftoverResults() const {
    return leftoverResults;
}

void CuttingOptimizerModel::optimize() {
    plans.clear();
    leftoverResults.clear();
    int currentOpId = nextOptimizationId++;

    // 🔧 Minden vágandó darabot kigyűjtünk darabonként — külön hosszal
    QVector<PieceWithMaterial> pieces;
    for (const auto& req : requests)
        for (int i = 0; i < req.quantity; ++i)
            pieces.append({ req.requiredLength, req.materialId });

    int rodId = 0;

    // 🔁 Addig keresünk rudat, amíg van vágandó darab
    while (!pieces.isEmpty()) {
        PieceWithMaterial target = pieces.front();
        QSet<QUuid> groupIds = GroupUtils::groupMembers(target.materialId);

        // 🏷️ Az aktuálisan kiválasztott rúd metaadatai
        QUuid selectedMaterialId;
        int selectedLength = 0;
        QVector<int> selectedCombo;
        bool found = false;
        bool usedReusable = false;

        // ♻️ Megpróbálunk találni hullóból újravágható rudat
        std::optional<ReusableCandidate> candidate =
            findBestReusableFit(reusableInventory, pieces, target.materialId);
        if (candidate.has_value()) {
            const auto& best = *candidate;

            selectedMaterialId = best.stock.materialId;
            selectedLength     = best.stock.availableLength_mm;
            selectedCombo      = best.combo;

            reusableInventory.remove(best.indexInInventory); // ❌ már nincs darabszám, kihúzzuk
            usedReusable = true;
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

                    QVector<int> pieceLengths;
                    for (const auto& p : pieces)
                        if (groupIds.contains(p.materialId))
                            pieceLengths.append(p.length);

                    selectedCombo = findBestFit(pieceLengths, selectedLength);
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
        for (int len : selectedCombo) {
            for (int i = 0; i < pieces.size(); ++i) {
                if (pieces[i].length == len && groupIds.contains(pieces[i].materialId)) {
                    pieces.removeAt(i);
                    break;
                }
            }
        }

        // 📦 Vágási terv mentése
        int totalCut = std::accumulate(selectedCombo.begin(), selectedCombo.end(), 0);
        int kerfTotal = (selectedCombo.size() - 1) * kerf; // vágási veszteség
        int used = totalCut + kerfTotal;
        int waste = selectedLength - used;

        QString barcode;
        if (usedReusable && candidate.has_value()) {
            barcode = candidate->stock.reusableBarcode(); // 🧾 egyedi azonosító a reusable darabra
        } else {
            const auto& masterOpt = MaterialRegistry::instance().findById(selectedMaterialId);
            barcode = masterOpt ? masterOpt->barcode : "(nincs barcode)";
        }

        CutPlan p{ ++rodId, selectedCombo, kerfTotal, waste, selectedMaterialId, barcode };
        plans.append(p);

        // ➕ Maradék mentése, ha >300 mm — az újrafelhasználható
        if (waste >= 300) {
            CutResult result;
            result.materialId     = selectedMaterialId;
            result.length         = selectedLength;
            result.cuts           = selectedCombo;
            result.waste          = waste;
            result.source         = usedReusable ? LeftoverSource::Manual : LeftoverSource::Optimization;
            result.optimizationId = usedReusable ? std::nullopt : std::make_optional(currentOpId);
            result.reusableBarcode = QString("RST-%1").arg(QUuid::createUuid().toString().mid(1, 6)); // 📛 egyedi azonosító

            leftoverResults.append(result);
        }
    }
}



/*
Sok darabot preferál	1000 vagy több
Kis hulladékot preferál	100–300
Kiegyensúlyozott	500–800
*/

QVector<int> CuttingOptimizerModel::findBestFit(const QVector<int>& available, int lengthLimit) const {
    QVector<int> bestCombo;
    int bestScore = std::numeric_limits<int>::min();
    int n = available.size();
    int totalCombos = 1 << n;

    const int weight = 1000; // súly a darabszámhoz

    for (int mask = 1; mask < totalCombos; ++mask) {
        QVector<int> combo;
        int total = 0;
        int count = 0;

        for (int i = 0; i < n; ++i) {
            if (mask & (1 << i)) {
                combo.append(available[i]);
                total += available[i];
                ++count;
            }
        }

        if (count == 0)
            continue;

        int kerfTotal = (count - 1) * kerf;
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
std::optional<CuttingOptimizerModel::ReusableCandidate> CuttingOptimizerModel::findBestReusableFit(
    const QVector<ReusableStockEntry>& reusableInventory,
    const QVector<PieceWithMaterial>& pieces,
    QUuid materialId
    ) const {
    std::optional<ReusableCandidate> best;
    int bestScore = std::numeric_limits<int>::min();
    QSet<QUuid> groupIds = GroupUtils::groupMembers(materialId);

    QVector<int> pieceLengths;
    for (const auto& p : pieces)
        if (groupIds.contains(p.materialId))
            pieceLengths.append(p.length);

    QVector<ReusableStockEntry> sorted = reusableInventory;
    std::sort(sorted.begin(), sorted.end(), [](const ReusableStockEntry& a, const ReusableStockEntry& b) {
        return a.availableLength_mm < b.availableLength_mm;
    });

    for (int i = 0; i < sorted.size(); ++i) {
        const auto& stock = sorted[i];
        if (!groupIds.contains(stock.materialId))
            continue;

        QVector<int> combo = findBestFit(pieceLengths, stock.availableLength_mm);
        if (combo.isEmpty())
            continue;

        int totalCut = std::accumulate(combo.begin(), combo.end(), 0);
        int kerfTotal = (combo.size() - 1) * kerf;
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




void CuttingOptimizerModel::setLeftovers(const QVector<CutResult> &list) {
    leftoverResults = list;
}

void CuttingOptimizerModel::clearLeftovers() {
    leftoverResults.clear();
}

void CuttingOptimizerModel::setRequests(const QVector<CuttingRequest>& list) {
    requests = list;
}

void CuttingOptimizerModel::setStockInventory(const QVector<StockEntry>& list) {
    profileInventory = list;
}

/**/

void CuttingOptimizerModel::setReusableInventory(const QVector<ReusableStockEntry>& reusable) {
    reusableInventory = reusable;
}

/**/

// void CuttingOptimizerModel::optimize() {
//     plans.clear();
//     leftoverResults.clear();
//     int currentOpId = nextOptimizationId++;

//     // 🔧 Az összes vágandó darab darabonként kigyűjtve
//     QVector<PieceWithMaterial> pieces;
//     for (const auto& req : requests)
//         for (int i = 0; i < req.quantity; ++i)
//             pieces.append({ req.requiredLength, req.materialId });

//     int rodId = 0;

//     // 🔁 Amíg van még darab, új rudat választunk és optimalizálunk
//     while (!pieces.isEmpty()) {
//         PieceWithMaterial target = pieces.front();

//         StockEntry selectedStock;
//         QVector<int> selectedCombo;
//         bool found = false;
//         bool usedReusable = false;

//         // ♻️ Próbáljuk a reusable készletet – optimalizált vágási kombinációval
//         auto candidate = findBestReusableFit(reusableInventory, pieces, target.materialId);
//         if (candidate.has_value()) {
//             const auto& best = *candidate;
//             selectedStock = best.stock;
//             selectedCombo = best.combo;
//             reusableInventory[best.indexInInventory].quantity--;
//             usedReusable = true;
//             found = true;
//         }

//         // 🧱 Ha nincs megfelelő reusable, próbáljuk a normál készletet
//         if (!found) {
//             for (int i = 0; i < profileInventory.size(); ++i) {
//                 if (profileInventory[i].materialId == target.materialId && profileInventory[i].quantity > 0) {
//                     selectedStock = profileInventory[i];
//                     profileInventory[i].quantity--;
//                     found = true;

//                     // 🧮 Keresünk legjobb kombinációt ehhez a stock rúdhoz
//                     QVector<int> pieceLengths;
//                     for (const auto& p : pieces)
//                         if (p.materialId == selectedStock.materialId)
//                             pieceLengths.append(p.length);

//                     selectedCombo = findBestFit(pieceLengths, selectedStock.stockLength_mm);
//                     break;
//                 }
//             }
//         }

//         // 🚫 Nincs elérhető rúd vagy nincs érvényes kombináció
//         if (!found || selectedCombo.isEmpty()) {
//             pieces.removeOne(target);
//             continue;
//         }

//         // 🧹 Kivágott darabok törlése a listából
//         for (int len : selectedCombo) {
//             for (int i = 0; i < pieces.size(); ++i) {
//                 if (pieces[i].length == len && pieces[i].materialId == selectedStock.materialId) {
//                     pieces.removeAt(i);
//                     break;
//                 }
//             }
//         }

//         // 📦 Vágási terv rögzítése
//         int totalCut = std::accumulate(selectedCombo.begin(), selectedCombo.end(), 0);
//         int kerfTotal = (selectedCombo.size() - 1) * kerf;
//         int used = totalCut + kerfTotal;
//         int waste = selectedStock.stockLength_mm - used;

//         plans.append({ ++rodId, selectedCombo, kerfTotal, waste, selectedStock.materialId });

//         // ➕ Maradék mentése, ha van értelme (>300 mm)
//         if (waste >= 300) {
//             CutResult result;
//             result.materialId     = selectedStock.materialId;
//             result.length         = selectedStock.stockLength_mm;
//             result.cuts           = selectedCombo;
//             result.waste          = waste;
//             result.source         = usedReusable ? LeftoverSource::Manual : LeftoverSource::Optimization;
//             result.optimizationId = usedReusable ? std::nullopt : std::make_optional(currentOpId);

//             leftoverResults.append(result);

//         }
//     }
// }
