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

        int waste = 0;
        int used = 0;
        int kerfTotal = 0;
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
        qint64 totalDPElapsed_us = 0;
        qint64 totalGreedyElapsed_us = 0;

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

        void accumulate(const FitResult& fr) {
            calls++;
            totalPicked += fr.pieceCount;
            totalWaste += fr.waste;
            maxWaste = std::max(maxWaste, (long)fr.waste);
            minWaste = std::min(minWaste, (long)fr.waste);

            totalKerf += fr.kerfTotal;
            totalUsed += fr.used;
            totalElapsed_us += fr.elapsed_us;

            switch (fr.strategy) {
            case FitResult::Strategy::FullFit:
                fullFit++;
                break;
            case FitResult::Strategy::BruteForce:
                bruteForce++;
                totalBFElapsed_us += fr.elapsed_us;
                bf_totalCombos    += fr.bf_totalCombos;
                bf_totalEvaluated += fr.bf_evaluated;
                bf_totalSkipped   += fr.bf_skipped;
                break;
            case FitResult::Strategy::DPPlain:
                dpPlain++;
                totalDPElapsed_us += fr.elapsed_us;
                dp_totalLimit      += fr.dpLimit;
                dp_totalFilledCells+= fr.dp_filledCells;
                dp_totalChainLength+= fr.dp_chainLength;
                break;
            case FitResult::Strategy::DPScoring:
                dpScoring++;
                totalDPElapsed_us += fr.elapsed_us;
                dp_totalLimit      += fr.dpLimit;
                dp_totalFilledCells+= fr.dp_filledCells;
                dp_totalChainLength+= fr.dp_chainLength;
                break;
            case FitResult::Strategy::Greedy:
                greedy++;
                totalGreedyElapsed_us += fr.elapsed_us;
                greedy_totalSortedSize += fr.greedy_sortedSize;
                greedy_totalPicks      += fr.greedy_picks;
                break;
                }
        }
    };

};




