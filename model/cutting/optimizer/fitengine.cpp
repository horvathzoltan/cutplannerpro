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
    zInfo(QString("🔍 FitEngine — legjobb combo keresése (n=%1, limit=%2 mm, kerf=%3)")
              .arg(available.size())
              .arg(lengthLimit)
              .arg(kerf_mm));

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
        zInfo(QString("🎯 FitEngine találat — üres combo (strategy=%1, used=%2, waste=%3)")
                  .arg(fr.strategyString())
                  .arg(fr.used)
                  .arg(fr.waste));
        return fr;
    }

    // 1) Gyors visszatérés: ha a teljes hossz + maximális kerf is belefér
    auto info = OptimizerUtils::computePhysicalCut(available, kerf_mm, lengthLimit);
    double totalRelevant   = info.totalCut;    // darabok hossza
    double kerfTotalFull   = info.kerfTotal;   // fizikai kerf

    // ✅ csak a darabok hosszát hasonlítjuk a limithez
    if (totalRelevant <= lengthLimit) {
        fr.strategy   = FitResult::Strategy::FullFit;
        fr.combo      = available;
        fr.pieceCount = n;
        fr.kerfTotal  = kerfTotalFull;
        fr.used       = totalRelevant;              // DP-szempontból: darabhossz
        fr.waste      = lengthLimit - fr.used;      // waste = hasznos hossz - darabhossz
        fr.elapsed_us = timer.nsecsElapsed() / 1000;
        zInfo(QString("🎯 FitEngine találat — FULL_FIT (picks=%1, used=%2, waste=%3)")
                  .arg(fr.pieceCount)
                  .arg(fr.used)
                  .arg(fr.waste));
        return fr;
    }

    // 2) Ha n <= 20 → brute-force
    if (n <= 20) {
        zInfo(QString("🔎 FitEngine keresés — stratégia=BRUTE_FORCE (n=%1, limit=%2, kerf=%3)")
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


            auto info = OptimizerUtils::computePhysicalCut(combo, kerf_mm, lengthLimit);
            double totalCut    = info.totalCut;   // darabok hossza
            double usedNoKerf  = totalCut;        // DP-limithez ezt használjuk

            if (usedNoKerf > lengthLimit) {
                skipped++;
                continue;
            }

            evaluated++;

            int waste          = static_cast<int>(lengthLimit - usedNoKerf);
            int leftoverLength = waste;
            int score          = OptimizerUtils::calcScore(combo.size(), waste, leftoverLength);


            if (score > bestScore) {
                bestScore  = score;
                bestCombo  = combo;
            }
        }

        auto info = OptimizerUtils::computePhysicalCut(bestCombo, kerf_mm, lengthLimit);
        double kerfTotal   = info.kerfTotal;
        double totalCut    = info.totalCut;   // darabok hossza
        double usedNoKerf  = totalCut;

        fr.strategy        = FitResult::Strategy::BruteForce;
        fr.combo           = bestCombo;
        fr.pieceCount      = bestCombo.size();
        fr.kerfTotal       = kerfTotal;
        fr.used            = usedNoKerf;                      // DP-szempontból: darabhossz
        fr.waste           = lengthLimit - fr.used;
        fr.bf_n            = n;
        fr.bf_totalCombos  = totalCombos;
        fr.bf_evaluated    = evaluated;
        fr.bf_skipped      = skipped;
        fr.elapsed_us      = timer.nsecsElapsed() / 1000;
        zInfo(QString("🎯 FitEngine találat — BRUTE_FORCE (picks=%1, used=%2, waste=%3, evaluated=%4, skipped=%5)")
                  .arg(fr.pieceCount)
                  .arg(fr.used)
                  .arg(fr.waste)
                  .arg(fr.bf_evaluated)
                  .arg(fr.bf_skipped));
        return fr;
    }

    // 3) DP fallback (nagy n, de kezelhető lengthLimit)
    if (lengthLimit <= 10000) {
        zInfo(QString("🔎 FitEngine keresés — stratégia=DP (n=%1, limit=%2, kerf=%3)")
                  .arg(n)
                  .arg(lengthLimit)
                  .arg(kerf_mm));

        double maxKerfDP = (n-1)* kerf_mm;//OptimizerUtils::roundKerfLoss(n, kerf_mm);
        double dpLimit   = lengthLimit - maxKerfDP;
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

        auto info = OptimizerUtils::computePhysicalCut(result, kerf_mm, lengthLimit);
        double kerfTotal   = info.kerfTotal;
        double totalCut    = info.totalCut;   // darabok hossza
        double usedNoKerf  = totalCut;

        if (usedNoKerf <= lengthLimit) {
            fr.strategy        = FitResult::Strategy::DPPlain;
            fr.combo           = result;
            fr.pieceCount      = result.size();
            fr.kerfTotal       = kerfTotal;
            fr.used            = usedNoKerf;
            fr.waste           = lengthLimit - fr.used;
            fr.dpLimit         = dpLimit;
            fr.dp_filledCells  = filledCells;
            fr.dp_chainLength  = result.size();
            fr.elapsed_us      = timer.nsecsElapsed() / 1000;
            zInfo(QString("🎯 FitEngine találat — DP_PLAIN (picks=%1, used=%2, waste=%3, dpLimit=%4, filled=%5)")
                      .arg(fr.pieceCount)
                      .arg(fr.used)
                      .arg(fr.waste)
                      .arg(fr.dpLimit)
                      .arg(fr.dp_filledCells));
            return fr;
        }

        // --- DP SCORING PATCH ---
        QVector<Cutting::Piece::PieceWithMaterial> bestCombo;
        int bestScore = std::numeric_limits<int>::min();


        // int kerfStep = OptimizerUtils::roundKerfLoss_1(1, kerf_mm);
        // if (kerfStep <= 0) kerfStep = 1;
        // DP scoring window lépésköz (mm-ben, DP index térben)
        // Nem fizikai kerf, csak ésszerű step a bestW körüli kereséshez.
        int kerfStep = static_cast<int>(std::ceil(kerf_mm));
        if (kerfStep <= 0)
            kerfStep = 1;

        for (int w2 = bestW; w2 >= 0 && w2 >= bestW - 5 * kerfStep; w2 -= kerfStep) {
            if (dp[w2] < 0) continue;

            auto combo = reconstruct(w2);

            auto info      = OptimizerUtils::computePhysicalCut(combo, kerf_mm, lengthLimit);
            double totalCut = info.totalCut;      // darabok hossza
            double usedNoKerf2 = totalCut;

            if (usedNoKerf2 > lengthLimit) continue;

            int waste2          = static_cast<int>(lengthLimit - usedNoKerf2);
            int leftoverLength2 = waste2;
            int score2          = OptimizerUtils::calcScore(combo.size(), waste2, leftoverLength2);

            if (score2 > bestScore) {
                bestScore = score2;
                bestCombo = combo;
            }
        }

        auto info2       = OptimizerUtils::computePhysicalCut(bestCombo, kerf_mm, lengthLimit);
        double kerftotal2 = info2.kerfTotal;
        double totalCut2  = info2.totalCut;
        double usedNoKerf2 = totalCut2;

        if (!bestCombo.isEmpty()) {
            fr.strategy        = FitResult::Strategy::DPScoring;
            fr.combo           = bestCombo;
            fr.pieceCount      = bestCombo.size();
            fr.kerfTotal       = kerftotal2;
            fr.used            = usedNoKerf2;
            fr.waste           = lengthLimit - fr.used;
            fr.dpLimit         = dpLimit;
            fr.dp_filledCells  = filledCells;
            fr.dp_chainLength  = bestCombo.size();
            fr.elapsed_us      = timer.nsecsElapsed() / 1000;
            zInfo(QString("🎯 FitEngine találat — DP_SCORING (picks=%1, used=%2, waste=%3, dpLimit=%4, filled=%5)")
                      .arg(fr.pieceCount)
                      .arg(fr.used)
                      .arg(fr.waste)
                      .arg(fr.dpLimit)
                      .arg(fr.dp_filledCells));
            return fr;
        }

        // ha idáig eljutunk, DP nem talált érvényeset → megyünk greedy‑re
    }

    // 4) Greedy fallback
    zInfo(QString("🔎 FitEngine keresés — stratégia=GREEDY (n=%1, limit=%2, kerf=%3)")
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
    double usedGreedy = 0;

    for (const auto& p : sorted) {
        QVector<Cutting::Piece::PieceWithMaterial> candidate = greedy;
        candidate.append(p);

        auto info       = OptimizerUtils::computePhysicalCut(candidate, kerf_mm, lengthLimit);
        double totalCut = info.totalCut;          // darabok hossza
        double candidateUsedNoKerf = totalCut;

        if (candidateUsedNoKerf <= lengthLimit) {
            greedy      = candidate;
            usedGreedy  = candidateUsedNoKerf;
        }
    }

    auto info3        = OptimizerUtils::computePhysicalCut(greedy, kerf_mm, lengthLimit);
    double kerfTotal3 = info3.kerfTotal;
    double totalCut3  = info3.totalCut;
    double usedNoKerf3 = totalCut3;

    fr.strategy           = FitResult::Strategy::Greedy;
    fr.combo              = greedy;
    fr.pieceCount         = greedy.size();
    fr.kerfTotal          = kerfTotal3;
    fr.used               = usedNoKerf3;
    fr.waste              = lengthLimit - fr.used;

    fr.greedy_sortedSize  = sorted.size();
    fr.greedy_picks       = greedy.size();
    fr.elapsed_us         = timer.nsecsElapsed() / 1000;

    zInfo(QString("🎯 FitEngine találat — GREEDY (picks=%1, used=%2, waste=%3, sorted=%4)")
              .arg(fr.pieceCount)
              .arg(fr.used)
              .arg(fr.waste)
              .arg(fr.greedy_sortedSize));

    return fr;

}
