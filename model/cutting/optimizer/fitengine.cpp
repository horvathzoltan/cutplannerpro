#include "fitengine.h"
#include "service/cutting/optimizer/optimizerutils.h"
#include "materials/utils/material_group_utils.h"
#include <QCoreApplication>
#include <algorithm>
#include <limits>

QVector<Cutting::Piece::PieceWithMaterial>
FitEngine::findBestFit(const QVector<Cutting::Piece::PieceWithMaterial>& available,
                       int lengthLimit,
                       double kerf_mm)
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

                // UI micro-yield (nagyon olcsó, de életmentő)
            if ((mask & 0x3FFF) == 0) { // minden 16384. iterációban
                QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents, 1);
            }

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

        // --- DP SCORING PATCH ---

        // Helper a DP visszafejtéshez
        auto reconstruct = [&](int w) {
            QVector<Cutting::Piece::PieceWithMaterial> combo;
            while (w > 0 && parent[w] != -1) {
                int idx = parent[w];
                combo.append(items[idx].piece);
                w -= items[idx].len;
            }
            return combo;
        };

        QVector<Cutting::Piece::PieceWithMaterial> bestCombo;
        int bestScore = std::numeric_limits<int>::min();

        // Egy kerf lépés (1 darabnyi kerf)
        int kerfStep = OptimizerUtils::roundKerfLoss(1, kerf_mm);
        if (kerfStep <= 0) kerfStep = 1;

        // Vizsgáljuk meg a bestW környékét (bestW, bestW-kerf, bestW-2*kerf, ...)
        for (int w2 = bestW; w2 >= 0 && w2 >= bestW - 5 * kerfStep; w2 -= kerfStep) {
            if (dp[w2] < 0) continue;

            auto combo = reconstruct(w2);

            int kerfTotal2 = OptimizerUtils::roundKerfLoss(combo.size(), kerf_mm);
            int used2 = OptimizerUtils::sumLengths(combo) + kerfTotal2;
            if (used2 > lengthLimit) continue;

            int waste2 = lengthLimit - used2;
            int score2 = OptimizerUtils::calcScore(combo.size(), waste2, waste2);

            if (score2 > bestScore) {
                bestScore = score2;
                bestCombo = combo;
            }
        }

        if (!bestCombo.isEmpty())
            return bestCombo;

        // --- DP SCORING PATCH END ---


        // greedy fallback
    }

    // 4) Greedy fallback (nagyon nagy n vagy nagy lengthLimit)
    auto sorted = available;

    // <<< IDE JÖN A GREEDY SCORING PATCH >>>
    // A darabokat nem csak hossz szerint rendezzük,
    // hanem a brute-force scoring alapján is priorizáljuk.
    std::sort(sorted.begin(), sorted.end(),
              [&](const auto& a, const auto& b){

                  if (a.info.length_mm != b.info.length_mm)
                      return a.info.length_mm > b.info.length_mm;

                  int wasteA = lengthLimit - a.info.length_mm;
                  int wasteB = lengthLimit - b.info.length_mm;

                  int scoreA = OptimizerUtils::calcScore(1, wasteA, wasteA);
                  int scoreB = OptimizerUtils::calcScore(1, wasteB, wasteB);

                  return scoreA > scoreB;
              });
    // <<< PATCH END >>>

    // std::sort(sorted.begin(), sorted.end(),
    //           [](const auto& a, const auto& b){
    //               return a.info.length_mm > b.info.length_mm;
    //           });

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


