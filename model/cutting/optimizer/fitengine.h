#pragma once

#include <QVector>
#include "../piece/piecewithmaterial.h"

class FitEngine {
public:

    struct FitResult {
        QVector<Cutting::Piece::PieceWithMaterial> combo;

        enum class Strategy {
            FullFit,
            BruteForce,
            DPPlain,
            DPScoring,
            Greedy
        } strategy;


        // Qt Creator hint: place after struct FitResult
        QString strategyString() {
            switch (strategy) {
            case Strategy::FullFit:     return "FullFit";
            case Strategy::BruteForce:  return "BruteForce";
            case Strategy::DPPlain:     return "DPPlain";
            case Strategy::DPScoring:   return "DPScoring";
            case Strategy::Greedy:      return "Greedy";
            }
            return "Unknown";
        }

        double waste = 0;
        double used = 0;
        double kerfTotal = 0;
        int pieceCount = 0;

        // időprofil
        qint64 elapsed_us = 0;

        // bruteforce stat
        int bf_n = 0;
        int bf_totalCombos = 0;
        int bf_evaluated = 0;
        int bf_skipped = 0;

        // DP stat
        int dpLimit = 0;
        int dp_filledCells = 0;
        int dp_chainLength = 0;

        // Greedy stat
        int greedy_sortedSize = 0;
        int greedy_picks = 0;
    };

    /**
 * @brief Megkeresi a legjobb darabkombinációt egy adott rúdhoz.
 *
 * Mit csinál?
 * - Kap egy listát a rendelkezésre álló darabokról (ugyanabból az anyagcsoportból),
 * - Kap egy hosszkorlátot (a rúd teljes hossza),
 * - Kap egy kerf értéket (a vágások miatti veszteség).
 *
 * Ezután végigpróbálja a darabok különböző kombinációit, és kiválasztja azt,
 * amelyik a legjobban kihasználja a rudat:
 * - minél több darabot sikerül kivágni,
 * - minél kevesebb hulladék marad.
 *
 * Röviden: ez a "kombináció-kereső", ami megmondja,
 * hogy mely darabokat érdemes együtt kivágni egy rúdból.
 */

    static FitResult
    findBestFit(const QVector<Cutting::Piece::PieceWithMaterial>& available,
                int lengthLimit,
                double kerf_mm);

    struct FitEngineTelemetry {
        long calls = 0;

        long fullFit = 0;
        long bruteForce = 0;
        long dpPlain = 0;
        long dpScoring = 0;
        long greedy = 0;

        long fullFitFound = 0;
        long bruteForceFound = 0;
        long dpPlainFound = 0;
        long dpScoringFound = 0;
        long greedyFound = 0;

        long totalPicked = 0;
        long totalWaste = 0;
        long maxWaste = 0;
        long minWaste = LONG_MAX;


        // kerf / used
        long totalKerf = 0;
        long totalUsed = 0;

        // időprofil összesítve
        qint64 totalElapsed_us = 0;
        qint64 totalBFElapsed_us = 0;
        qint64 totalDPPlainElapsed_us = 0;
        qint64 totalDPScoringElapsed_us = 0;
        qint64 totalGreedyElapsed_us = 0;
        qint64 totalFullFitElapsed_us = 0;

        // bruteforce aggregált
        long bf_totalCombos = 0;
        long bf_totalEvaluated = 0;
        long bf_totalSkipped = 0;

        // DP aggregált
        long dp_totalLimit = 0;
        long dp_totalFilledCells = 0;
        long dp_totalChainLength = 0;

        // Greedy aggregált
        long greedy_totalSortedSize = 0;
        long greedy_totalPicks = 0;


        // leftover statisztika algoritmusonként
        long fullFit_goodCount = 0;
        long fullFit_goodTotal = 0;
        long fullFit_badCount = 0;
        long fullFit_badTotal = 0;
        long fullFit_oversizeCount = 0;
        long fullFit_oversizeTotal = 0;

        long bruteForce_goodCount = 0;
        long bruteForce_goodTotal = 0;
        long bruteForce_badCount = 0;
        long bruteForce_badTotal = 0;
        long bruteForce_oversizeCount = 0;
        long bruteForce_oversizeTotal = 0;

        long dpPlain_goodCount = 0;
        long dpPlain_goodTotal = 0;
        long dpPlain_badCount = 0;
        long dpPlain_badTotal = 0;
        long dpPlain_oversizeCount = 0;
        long dpPlain_oversizeTotal = 0;

        long dpScoring_goodCount = 0;
        long dpScoring_goodTotal = 0;
        long dpScoring_badCount = 0;
        long dpScoring_badTotal = 0;
        long dpScoring_oversizeCount = 0;
        long dpScoring_oversizeTotal = 0;

        long greedy_goodCount = 0;
        long greedy_goodTotal = 0;
        long greedy_badCount = 0;
        long greedy_badTotal = 0;
        long greedy_oversizeCount = 0;
        long greedy_oversizeTotal = 0;


        void accumulate(const FitResult& fr) {
            calls++;
            totalPicked += fr.pieceCount;
            totalWaste += fr.waste;
            maxWaste = std::max(maxWaste, (long)fr.waste);
            minWaste = std::min(minWaste, (long)fr.waste);

            totalKerf += fr.kerfTotal;
            totalUsed += fr.used;
            totalElapsed_us += fr.elapsed_us;

            bool hasHit = (fr.pieceCount > 0);

            auto classifyLeftover = [&](int waste,
                                        long& goodCount, long& goodTotal,
                                        long& badCount, long& badTotal,
                                        long& overCount, long& overTotal)
            {
                if (waste >= 500 && waste <= 800) {
                    goodCount++;
                    goodTotal += waste;
                } else if (waste > 0 && waste < 300) {
                    badCount++;
                    badTotal += waste;
                } else if (waste > 800) {
                    overCount++;
                    overTotal += waste;
                }
            };


            switch (fr.strategy) {
            case FitResult::Strategy::FullFit:
                fullFit++;
                if (hasHit) fullFitFound++;
                totalFullFitElapsed_us += fr.elapsed_us;

                classifyLeftover(fr.waste,
                                 fullFit_goodCount, fullFit_goodTotal,
                                 fullFit_badCount, fullFit_badTotal,
                                 fullFit_oversizeCount, fullFit_oversizeTotal);
                break;

            case FitResult::Strategy::BruteForce:
                bruteForce++;
                if (hasHit) bruteForceFound++;
                totalBFElapsed_us += fr.elapsed_us;
                bf_totalCombos    += fr.bf_totalCombos;
                bf_totalEvaluated += fr.bf_evaluated;
                bf_totalSkipped   += fr.bf_skipped;

                classifyLeftover(fr.waste,
                                 bruteForce_goodCount, bruteForce_goodTotal,
                                 bruteForce_badCount, bruteForce_badTotal,
                                 bruteForce_oversizeCount, bruteForce_oversizeTotal);
                break;

            case FitResult::Strategy::DPPlain:
                dpPlain++;
                if (hasHit) dpPlainFound++;
                totalDPPlainElapsed_us += fr.elapsed_us;
                dp_totalLimit      += fr.dpLimit;
                dp_totalFilledCells+= fr.dp_filledCells;
                dp_totalChainLength+= fr.dp_chainLength;

                classifyLeftover(fr.waste,
                                 dpPlain_goodCount, dpPlain_goodTotal,
                                 dpPlain_badCount, dpPlain_badTotal,
                                 dpPlain_oversizeCount, dpPlain_oversizeTotal);
                break;

            case FitResult::Strategy::DPScoring:
                dpScoring++;
                if (hasHit) dpScoringFound++;
                totalDPScoringElapsed_us += fr.elapsed_us;
                dp_totalLimit      += fr.dpLimit;
                dp_totalFilledCells+= fr.dp_filledCells;
                dp_totalChainLength+= fr.dp_chainLength;

                classifyLeftover(fr.waste,
                                 dpScoring_goodCount, dpScoring_goodTotal,
                                 dpScoring_badCount, dpScoring_badTotal,
                                 dpScoring_oversizeCount, dpScoring_oversizeTotal);
                break;

            case FitResult::Strategy::Greedy:
                greedy++;
                if (hasHit) greedyFound++;
                totalGreedyElapsed_us += fr.elapsed_us;
                greedy_totalSortedSize += fr.greedy_sortedSize;
                greedy_totalPicks      += fr.greedy_picks;

                classifyLeftover(fr.waste,
                                 greedy_goodCount, greedy_goodTotal,
                                 greedy_badCount, greedy_badTotal,
                                 greedy_oversizeCount, greedy_oversizeTotal);
                break;
            }

        }
    };

};




