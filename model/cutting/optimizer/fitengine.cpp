#include "fitengine.h"
#include "service/cutting/optimizer/optimizerutils.h"
#include "materials/utils/material_group_utils.h"
#include "common/logger.h"
#include <QCoreApplication>
#include <QElapsedTimer>
#include <algorithm>
#include <limits>

FitEngine::FitResult
FitEngine::findBestFit(const QVector<Cutting::Piece::PieceWithMaterial>& available,
                       int lengthLimit,
                       double kerf_mm)
{
    FitResult fr;
    QElapsedTimer timer;
    timer.start();

    const int n = available.size();

    // 0) Ha nincs darab
    if (n == 0) {
        fr.strategy   = FitResult::Strategy::Greedy; // vagy bármi, de jelöljük
        fr.combo.clear();
        fr.pieceCount = 0;
        fr.kerfTotal  = 0;
        fr.used       = 0;
        fr.waste      = lengthLimit;
        fr.elapsed_us = timer.nsecsElapsed() / 1000;
        return fr;
    }

    // 1) Gyors visszatérés: ha a teljes hossz + maximális kerf is belefér
    int totalRelevant = OptimizerUtils::sumLengths(available);
    int maxKerf       = OptimizerUtils::roundKerfLoss(n, kerf_mm);
    if (totalRelevant + maxKerf <= lengthLimit) {
        fr.strategy   = FitResult::Strategy::FullFit;
        fr.combo      = available;
        fr.pieceCount = n;
        fr.kerfTotal  = maxKerf;
        fr.used       = totalRelevant + maxKerf;
        fr.waste      = lengthLimit - fr.used;
        fr.elapsed_us = timer.nsecsElapsed() / 1000;
        return fr;
    }

    // 2) Ha n <= 20 → brute-force
    if (n <= 20) {
        zInfo(QString("FitEngine::findBestFit strategy=BRUTE_FORCE n=%1 limit=%2 kerf=%3")
                  .arg(n)
                  .arg(lengthLimit)
                  .arg(kerf_mm));

        int bestScore   = std::numeric_limits<int>::min();
        int totalCombos = 1 << n;
        int evaluated   = 0;
        int skipped     = 0;
        QVector<Cutting::Piece::PieceWithMaterial> bestCombo;

        for (int mask = 1; mask < totalCombos; ++mask) {

            if ((mask & 0x3FFF) == 0) {
                QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents, 1);
            }

            QVector<Cutting::Piece::PieceWithMaterial> combo;
            combo.reserve(n);
            for (int i = 0; i < n; ++i)
                if (mask & (1 << i))
                    combo.append(available[i]);

            int totalCut = OptimizerUtils::sumLengths(combo);
            if (totalCut > lengthLimit) {
                skipped++;
                continue;
            }

            int kerfTotal = OptimizerUtils::roundKerfLoss(combo.size(), kerf_mm);
            int used      = totalCut + kerfTotal;

            if (used > lengthLimit) {
                skipped++;
                continue;
            }

            evaluated++;

            int waste = lengthLimit - used;
            int score = OptimizerUtils::calcScore(combo.size(), waste, waste);

            if (score > bestScore) {
                bestScore  = score;
                bestCombo  = combo;
            }
        }

        fr.strategy        = FitResult::Strategy::BruteForce;
        fr.combo           = bestCombo;
        fr.pieceCount      = bestCombo.size();
        fr.kerfTotal       = OptimizerUtils::roundKerfLoss(fr.pieceCount, kerf_mm);
        fr.used            = OptimizerUtils::sumLengths(bestCombo) + fr.kerfTotal;
        fr.waste           = lengthLimit - fr.used;
        fr.bf_n            = n;
        fr.bf_totalCombos  = totalCombos;
        fr.bf_evaluated    = evaluated;
        fr.bf_skipped      = skipped;
        fr.elapsed_us      = timer.nsecsElapsed() / 1000;
        return fr;
    }

    // 3) DP fallback (nagy n, de kezelhető lengthLimit)
    if (lengthLimit <= 10000) {
        zInfo(QString("FitEngine::findBestFit strategy=DP n=%1 limit=%2 kerf=%3")
                  .arg(n)
                  .arg(lengthLimit)
                  .arg(kerf_mm));

        int maxKerfDP = OptimizerUtils::roundKerfLoss(n, kerf_mm);
        int dpLimit   = lengthLimit - maxKerfDP;
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
            int len = items[i].len;
            for (int w = dpLimit; w >= len; --w) {
                if (dp[w - len] != -1 && dp[w - len] + len > dp[w]) {
                    dp[w]     = dp[w - len] + len;
                    parent[w] = i;
                }
            }
        }

        int bestW = 0;
        for (int w = 1; w <= dpLimit; ++w)
            if (dp[w] > dp[bestW])
                bestW = w;

        int filledCells = 0;
        for (int w = 0; w <= dpLimit; ++w)
            if (dp[w] >= 0)
                filledCells++;

        auto reconstruct = [&](int w) {
            QVector<Cutting::Piece::PieceWithMaterial> combo;
            while (w > 0 && parent[w] != -1) {
                int idx = parent[w];
                combo.append(items[idx].piece);
                w -= items[idx].len;
            }
            return combo;
        };

        QVector<Cutting::Piece::PieceWithMaterial> result = reconstruct(bestW);

        int kerfTotal = OptimizerUtils::roundKerfLoss(result.size(), kerf_mm);
        int used      = OptimizerUtils::sumLengths(result) + kerfTotal;
        if (used <= lengthLimit) {
            fr.strategy        = FitResult::Strategy::DPPlain;
            fr.combo           = result;
            fr.pieceCount      = result.size();
            fr.kerfTotal       = kerfTotal;
            fr.used            = used;
            fr.waste           = lengthLimit - used;
            fr.dpLimit         = dpLimit;
            fr.dp_filledCells  = filledCells;
            fr.dp_chainLength  = result.size();
            fr.elapsed_us      = timer.nsecsElapsed() / 1000;
            return fr;
        }

        // --- DP SCORING PATCH ---
        QVector<Cutting::Piece::PieceWithMaterial> bestCombo;
        int bestScore = std::numeric_limits<int>::min();


        int kerfStep = OptimizerUtils::roundKerfLoss(1, kerf_mm);
        if (kerfStep <= 0) kerfStep = 1;

        for (int w2 = bestW; w2 >= 0 && w2 >= bestW - 5 * kerfStep; w2 -= kerfStep) {
            if (dp[w2] < 0) continue;

            auto combo = reconstruct(w2);

            int kerfTotal2 = OptimizerUtils::roundKerfLoss(combo.size(), kerf_mm);
            int used2      = OptimizerUtils::sumLengths(combo) + kerfTotal2;
            if (used2 > lengthLimit) continue;

            int waste2 = lengthLimit - used2;
            int score2 = OptimizerUtils::calcScore(combo.size(), waste2, waste2);

            if (score2 > bestScore) {
                bestScore = score2;
                bestCombo = combo;
            }
        }

        if (!bestCombo.isEmpty()) {
            fr.strategy        = FitResult::Strategy::DPScoring;
            fr.combo           = bestCombo;
            fr.pieceCount      = bestCombo.size();
            fr.kerfTotal       = OptimizerUtils::roundKerfLoss(fr.pieceCount, kerf_mm);
            fr.used            = OptimizerUtils::sumLengths(bestCombo) + fr.kerfTotal;
            fr.waste           = lengthLimit - fr.used;
            fr.dpLimit         = dpLimit;
            fr.dp_filledCells  = filledCells;
            fr.dp_chainLength  = bestCombo.size();
            fr.elapsed_us      = timer.nsecsElapsed() / 1000;
            return fr;
        }

        // ha idáig eljutunk, DP nem talált érvényeset → megyünk greedy‑re
    }

    // 4) Greedy fallback
    zInfo(QString("FitEngine::findBestFit strategy=GREEDY n=%1 limit=%2 kerf=%3")
              .arg(n)
              .arg(lengthLimit)
              .arg(kerf_mm));

    auto sorted = available;

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

    QVector<Cutting::Piece::PieceWithMaterial> greedy;
    int usedGreedy = 0;

    for (const auto& p : sorted) {
        int kerf = greedy.isEmpty()
        ? 0
        : OptimizerUtils::roundKerfLoss(1, kerf_mm);

        int candidateUsed = usedGreedy + p.info.length_mm + kerf;

        if (candidateUsed <= lengthLimit) {
            greedy.append(p);
            usedGreedy = candidateUsed;
        }
    }

    fr.strategy           = FitResult::Strategy::Greedy;
    fr.combo              = greedy;
    fr.pieceCount         = greedy.size();
    fr.kerfTotal          = OptimizerUtils::roundKerfLoss(fr.pieceCount, kerf_mm);
    fr.used               = OptimizerUtils::sumLengths(greedy) + fr.kerfTotal;
    fr.waste              = lengthLimit - fr.used;
    fr.greedy_sortedSize  = sorted.size();
    fr.greedy_picks       = greedy.size();
    fr.elapsed_us         = timer.nsecsElapsed() / 1000;
    return fr;

}
