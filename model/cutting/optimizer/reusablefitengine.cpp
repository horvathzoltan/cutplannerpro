#include "fitengine.h"
#include "reusablefitengine.h"
#include "service/cutting/optimizer/optimizerutils.h"
#include "materials/utils/material_group_utils.h"

/*
♻️ A reusable rudak sorbarendezése garantálja, hogy előbb próbáljuk a kisebb, „kockáztathatóbb” rudakat
✂️ A pontszámítás továbbra is érvényes: preferáljuk a több darabot és a kisebb hulladékot
 */
std::optional<ReusableCandidate>
ReusableFitEngine::findBestReusableFit(const QVector<LeftoverStockEntry>& mergedView,
                                       int globalCount,
                                       const QVector<Cutting::Piece::PieceWithMaterial>& pieces,
                                       QUuid materialId,
                                       double kerf_mm,
                                       const QSet<QUuid>& usedLeftoverEntryIds,
                                       Cutting::Optimizer::OptimizerModel& model)
{
    std::optional<ReusableCandidate> best;
    int bestScore = std::numeric_limits<int>::min();
    QSet<QUuid> groupIds = GroupUtils::groupMembers(materialId);

    QVector<int> _aff_limits;
    QVector<int> _aff_results;


    // 🔎 releváns darabok kiszűrése
    QVector<Cutting::Piece::PieceWithMaterial> relevantPieces;
    for (const auto& p : pieces)
        if (groupIds.contains(p.materialId))
            relevantPieces.append(p);

    for (int i = 0; i < mergedView.size(); ++i) {
        const auto& stock = mergedView[i];

        zInfo(QString("leftoverLoop iteration #%1 → limit=%2")
                  .arg(i + 1)
                  .arg(stock.availableLength_mm));

        if (stock.used) continue;
        if (!groupIds.contains(stock.materialId)) continue;
        if (usedLeftoverEntryIds.contains(stock.entryId)) continue; // ezt a hullót már elhasználtuk

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
        FitEngine::FitResult fit =
            FitEngine::findBestFit(relevantPieces, stock.availableLength_mm, kerf_mm);

        model._fitTelemetry.accumulate(fit);

        zInfo(QString("    strategy=%1, picks=%2, waste=%3")
                  .arg(fit.strategyString())
                  .arg(fit.pieceCount)
                  .arg(fit.waste));

        if (fit.combo.isEmpty()) {
            zInfo("    result: FAILED");
            continue;
        }

        zInfo("    result: SUCCESS");

        _aff_limits.append(stock.availableLength_mm);
        _aff_results.append(fit.pieceCount);

        if (fit.combo.isEmpty()) continue;

        // A FitResult már tartalmazza:
        int used           = fit.used;
        int waste          = fit.waste;
        int leftoverLength = stock.availableLength_mm - used;

        if (used > stock.availableLength_mm) continue;


        int score = OptimizerUtils::calcScore(fit.combo.size(), waste, leftoverLength);

        if (score > bestScore) {
            bestScore = score;
            ReusableCandidate cand;
            cand.indexInView = i;
            cand.stock       = stock;
            cand.combo       = fit.combo;
            cand.waste       = waste;
            cand.source      = (i < globalCount)
                              ? ReusableCandidate::Source::GlobalSnapshot
                              : ReusableCandidate::Source::LocalPool;
            best = cand;
        }
    }

    QStringList limitsStr, resultsStr;
    for (int v : _aff_limits)  limitsStr << QString::number(v);
    for (int v : _aff_results) resultsStr << QString::number(v);

    zInfo(QString("🧩 ReusableFitEngine::findBestFit attempts=%1 limits=%2 results=%3")
              .arg(_aff_limits.size())
              .arg(limitsStr.join(","))
              .arg(resultsStr.join(",")));

    return best;
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

// QVector<Cutting::Piece::PieceWithMaterial>
// OptimizerModel::findBestFit(const QVector<Cutting::Piece::PieceWithMaterial>& available,
//                             int lengthLimit,
//                             double kerf_mm) const
// {
//     QVector<Cutting::Piece::PieceWithMaterial> bestCombo;
//     int n = available.size();

//     // 0) Ha nincs darab
//     if (n == 0)
//         return {};

//     // Gyors visszatérés: ha a teljes hossz + maximális kerf (darabszám × kerf) is belefér,
//     // akkor biztosan belefér a valóságos kerf-modellel is (ahol az első darab kerf=0).

//     int totalRelevant = OptimizerUtils::sumLengths(available);
//     int maxKerf = OptimizerUtils::roundKerfLoss(n, kerf_mm);
//     if (totalRelevant + maxKerf <= lengthLimit)
//         return available;

//     // 2) Ha n <= 20 → brute-force (tökéletes)
//     if (n <= 20) {
//         int bestScore = std::numeric_limits<int>::min();
//         int totalCombos = 1 << n;

//         for (int mask = 1; mask < totalCombos; ++mask) {
//             QVector<Cutting::Piece::PieceWithMaterial> combo;
//             for (int i = 0; i < n; ++i)
//                 if (mask & (1 << i))
//                     combo.append(available[i]);

//             int totalCut = OptimizerUtils::sumLengths(combo);
//             if (totalCut > lengthLimit) continue;

//             int kerfTotal = OptimizerUtils::roundKerfLoss(combo.size(), kerf_mm);
//             int used = totalCut + kerfTotal;

//             // 🔒 Overfill-védelem
//             if (used > lengthLimit)
//                 continue;

//             int waste = lengthLimit - used;
//             int score = OptimizerUtils::calcScore(combo.size(), waste, waste);

//             if (score > bestScore) {
//                 bestScore = score;
//                 bestCombo = combo;
//             }
//         }
//         return bestCombo;
//     }

//     // 3) DP fallback (nagy n, de kezelhető hosszLimit)
//     if (lengthLimit <= 10000) {
//         int maxKerf = OptimizerUtils::roundKerfLoss(n, kerf_mm);
//         int dpLimit = lengthLimit - maxKerf;
//         if (dpLimit < 0) dpLimit = 0;

//         struct Item { int len; Cutting::Piece::PieceWithMaterial piece; };
//         QVector<Item> items;
//         items.reserve(n);


//         for (const auto& p : available)
//             items.append({ p.info.length_mm, p });

//         QVector<int> dp(dpLimit + 1, -1);
//         QVector<int> parent(dpLimit + 1, -1);

//         dp[0] = 0;

//         for (int i = 0; i < n; ++i) {
//             int len = items[i].len; // kerf nélkül
//             for (int w = dpLimit; w >= len; --w) {
//                 if (dp[w - len] != -1 && dp[w - len] + len > dp[w]) {
//                     dp[w] = dp[w - len] + len;
//                     parent[w] = i;
//                 }
//             }
//         }

//         int bestW = 0;
//         for (int w = 1; w <= dpLimit; ++w)
//             if (dp[w] > dp[bestW])
//                 bestW = w;

//         QVector<Cutting::Piece::PieceWithMaterial> result;
//         int w = bestW;
//         while (w > 0 && parent[w] != -1) {
//             int idx = parent[w];
//             result.append(items[idx].piece);
//             w -= items[idx].len;
//         }

//         // kerf utólagos számítása
//         int kerfTotal = OptimizerUtils::roundKerfLoss(result.size(), kerf_mm);
//         int used = OptimizerUtils::sumLengths(result) + kerfTotal;
//         if (used <= lengthLimit)
//             return result;
//         // greedy fallback
//     }

//     // 4) Greedy fallback (nagyon nagy n vagy nagy lengthLimit)
//     auto sorted = available;
//     std::sort(sorted.begin(), sorted.end(),
//               [](const auto& a, const auto& b){
//                   return a.info.length_mm > b.info.length_mm;
//               });

//     QVector<Cutting::Piece::PieceWithMaterial> greedy;
//     int used = 0;

//     for (const auto& p : sorted) {
//         // első darab: nincs vágás → 0 kerf
//         // további darabok: minden új darabhoz 1 vágás → kerf_mm

//         int kerf = greedy.isEmpty()
//         ? 0
//         : OptimizerUtils::roundKerfLoss(1, kerf_mm); // egységes kerf: első darab 0, továbbiak 1×kerf

//         int candidateUsed = used + p.info.length_mm + kerf;

//         if (candidateUsed <= lengthLimit) {
//             greedy.append(p);
//             used = candidateUsed;
//         }
//     }

//     return greedy;
// }



/*
♻️ A reusable rudak sorbarendezése garantálja, hogy előbb próbáljuk a kisebb, „kockáztathatóbb” rudakat
✂️ A pontszámítás továbbra is érvényes: preferáljuk a több darabot és a kisebb hulladékot
 */
// std::optional<OptimizerModel::ReusableCandidate>
// OptimizerModel::findBestReusableFit(const QVector<LeftoverStockEntry>& mergedView,
//                                     int globalCount,
//                                     const QVector<Cutting::Piece::PieceWithMaterial>& pieces,
//                                     QUuid materialId,
//                                     double kerf_mm) const
// {
//     std::optional<ReusableCandidate> best;
//     int bestScore = std::numeric_limits<int>::min();
//     QSet<QUuid> groupIds = GroupUtils::groupMembers(materialId);

//     // 🔎 releváns darabok kiszűrése
//     QVector<Cutting::Piece::PieceWithMaterial> relevantPieces;
//     for (const auto& p : pieces)
//         if (groupIds.contains(p.materialId))
//             relevantPieces.append(p);

//     for (int i = 0; i < mergedView.size(); ++i) {
//         const auto& stock = mergedView[i];
//         if (stock.used) continue;
//         if (!groupIds.contains(stock.materialId)) continue;
//         if (_usedLeftoverEntryIds.contains(stock.entryId)) continue; // ezt a hullót már elhasználtuk

//         // PRIORITÁS: egy darab, ami pontosan elfogyasztja
//         const auto single = OptimizerUtils::findSingleExactFit(relevantPieces, stock.availableLength_mm, kerf_mm);
//         if (single.has_value()) {
//             int kerfTotal = OptimizerUtils::roundKerfLoss(1, kerf_mm);
//             int used = single->info.length_mm + kerfTotal;
//             int waste = OptimizerUtils::computeWasteInt(stock.availableLength_mm, used);
//             if (waste == 0) {
//                 return ReusableCandidate{ i, stock, QVector<Cutting::Piece::PieceWithMaterial>{ *single }, waste };
//             }
//         }

//         // Egyébként: keresd a legjobb részhalmazt
//         auto combo = FitEngine::findBestFit(relevantPieces, stock.availableLength_mm, kerf_mm);
//         zInfo(QString("findBestFit: %1 darab, bestCombo size=%2")
//                    .arg(relevantPieces.size()).arg(combo.size()));

//         if (combo.isEmpty()) continue;

//         int totalCut = OptimizerUtils::sumLengths(combo);
//         int kerfTotal = OptimizerUtils::roundKerfLoss(combo.size(), kerf_mm);
//         int used = totalCut + kerfTotal;
//         if (used > stock.availableLength_mm) continue; // early discard

//         int waste = OptimizerUtils::computeWasteInt(stock.availableLength_mm, used);
//         int leftoverLength = stock.availableLength_mm - used;


//         int score = OptimizerUtils::calcScore(combo.size(), waste, leftoverLength);

//         if (score > bestScore) {
//             bestScore = score;
//             ReusableCandidate cand;
//             cand.indexInView = i;
//             cand.stock = stock;
//             cand.combo = combo;
//             cand.waste = waste;
//             cand.source = (i < globalCount)
//                               ? ReusableCandidate::Source::GlobalSnapshot
//                               : ReusableCandidate::Source::LocalPool;
//             best = cand;
//         }
//     }
//     return best;
// }

/**
 * @brief Megkeresi, hogy a hulló készletből (reusableInventory) van-e olyan darab,
 *        amiből érdemes újra vágni.
 *
 * Mit csinál?
 * - Végignézi az összes hulló rudat (reusableInventory),
 * - Megnézi, hogy az adott anyagcsoport darabjai közül melyek férnek bele,
 * - Kiszámolja, mennyi darabot lehet belőle kivágni és mennyi hulladék marad,
 * - Kiválasztja a legjobb találatot (ahol a legtöbb darabot sikerül elhelyezni,
 *   és a legkevesebb a veszteség).
 *
 * Ha talál ilyet, visszaad egy ReusableCandidate-et, ami tartalmazza:
 * - melyik hulló rúd volt az,
 * - milyen darabokat sikerült belőle kivágni,
 * - mennyi hulladék maradt.
 *
 * Ha nem talál megfelelő hullót, akkor üres (std::nullopt) az eredmény.
 *
 * Röviden: ez a "hulló-vadász", ami megmondja,
 * hogy érdemes-e egy meglévő maradékból dolgozni, vagy sem.
 */
// std::optional<ReusableCandidate> findBestReusableFit(
//     const QVector<LeftoverStockEntry>& reusableInventory,
//     int globalCount,
//     const QVector<Cutting::Piece::PieceWithMaterial>& pieces,
//     QUuid materialId,
//     double kerf_mm
//     ) const;

