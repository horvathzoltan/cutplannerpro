#pragma once

#include <model/cutting/plan/cutplan.h>
#include "../../../common/logger.h"
#include "fitengine.h"

namespace Cutting {
namespace Optimizer {
namespace TelemetryHelper {

inline void logSummary(const FitEngine::FitEngineTelemetry& t)
{

    double avgPicked = (t.calls > 0)
                           ? double(t.totalPicked) / double(t.calls)
                           : 0.0;
    double avgWaste = (t.calls > 0)
                          ? double(t.totalWaste) / double(t.calls)
                          : 0.0;
    double avgKerf  = (t.calls > 0)
                         ? double(t.totalKerf) / double(t.calls)
                         : 0.0;
    double avgKerfRatio = (t.totalUsed > 0)
                              ? (double(t.totalKerf) / double(t.totalUsed)) * 100.0
                              : 0.0;

    double avgElapsed_ms = (t.calls > 0)
                               ? double(t.totalElapsed_us) / 1000.0 / double(t.calls)
                               : 0.0;
    double avgBF_ms = (t.bruteForce > 0)
                          ? double(t.totalBFElapsed_us) / 1000.0 / double(t.bruteForce)
                          : 0.0;
    double avgDP_ms = ((t.dpPlain + t.dpScoring) > 0)
                          ? double(t.totalDPPlainElapsed_us + t.totalDPScoringElapsed_us)
                                / 1000.0
                                / double(t.dpPlain + t.dpScoring)
                          : 0.0;

    double avgGreedy_ms = (t.greedy > 0)
                              ? double(t.totalGreedyElapsed_us) / 1000.0 / double(t.greedy)
                              : 0.0;

    double avgBFCombos = (t.bruteForce > 0)
                             ? double(t.bf_totalCombos) / double(t.bruteForce)
                             : 0.0;
    double avgBFEvaluated = (t.bruteForce > 0)
                                ? double(t.bf_totalEvaluated) / double(t.bruteForce)
                                : 0.0;
    double avgBFSkipped = (t.bruteForce > 0)
                              ? double(t.bf_totalSkipped) / double(t.bruteForce)
                              : 0.0;

    double avgDPLimit = ((t.dpPlain + t.dpScoring) > 0)
                            ? double(t.dp_totalLimit) / double(t.dpPlain + t.dpScoring)
                            : 0.0;
    double avgDPFilled = ((t.dpPlain + t.dpScoring) > 0)
                             ? double(t.dp_totalFilledCells) / double(t.dpPlain + t.dpScoring)
                             : 0.0;
    double avgDPChain = ((t.dpPlain + t.dpScoring) > 0)
                            ? double(t.dp_totalChainLength) / double(t.dpPlain + t.dpScoring)
                            : 0.0;

    double avgGreedySorted = (t.greedy > 0)
                                 ? double(t.greedy_totalSortedSize) / double(t.greedy)
                                 : 0.0;
    double avgGreedyPicks = (t.greedy > 0)
                                ? double(t.greedy_totalPicks) / double(t.greedy)
                                : 0.0;

    zInfo("📊 OPTIMALIZÁCIÓS TELEMETRIA — összegzés");
    zInfo(QString("   • Hívások száma: %1").arg(t.calls));
    zInfo(QString("   • FullFit: %1").arg(t.fullFit));
    zInfo(QString("   • BruteForce: %1").arg(t.bruteForce));
    zInfo(QString("   • DP (plain): %1").arg(t.dpPlain));
    zInfo(QString("   • DP (scoring): %1").arg(t.dpScoring));
    zInfo(QString("   • Greedy: %1").arg(t.greedy));


    zInfo(QString("   • Átlag pick count: %1").arg(avgPicked, 0, 'f', 2));
    zInfo(QString("   • Átlag waste: %1 mm").arg(avgWaste, 0, 'f', 2));
    zInfo(QString("   • Max waste: %1 mm").arg(t.maxWaste));
    zInfo(QString("   • Min waste: %1 mm").arg(t.minWaste));
    zInfo(QString("   • Átlag kerf: %1 mm").arg(avgKerf, 0, 'f', 2));
    zInfo(QString("   • Kerf arány: %1 %%").arg(avgKerfRatio, 0, 'f', 3));
    zInfo(QString("   • Átlag futási idő: %1 ms").arg(avgElapsed_ms, 0, 'f', 3));
    zInfo(QString("   • Átlag BF idő: %1 ms").arg(avgBF_ms, 0, 'f', 3));
    zInfo(QString("   • Átlag DP idő: %1 ms").arg(avgDP_ms, 0, 'f', 3));
    zInfo(QString("   • Átlag Greedy idő: %1 ms").arg(avgGreedy_ms, 0, 'f', 3));


    zInfo(QString("   • BF átlag kombinációk: %1").arg(avgBFCombos, 0, 'f', 2));
    zInfo(QString("   • BF átlag vizsgált: %1").arg(avgBFEvaluated, 0, 'f', 2));
    zInfo(QString("   • BF átlag skip: %1").arg(avgBFSkipped, 0, 'f', 2));
    zInfo(QString("   • DP átlag limit: %1").arg(avgDPLimit, 0, 'f', 2));
    zInfo(QString("   • DP átlag filled cells: %1").arg(avgDPFilled, 0, 'f', 2));
    zInfo(QString("   • DP átlag chain length: %1").arg(avgDPChain, 0, 'f', 2));
    zInfo(QString("   • Greedy átlag rendezett elemszám: %1").arg(avgGreedySorted, 0, 'f', 2));
    zInfo(QString("   • Greedy átlag picks: %1").arg(avgGreedyPicks, 0, 'f', 2));

    zInfo("📘 TELEMETRIA — vége");
}

} // namespace TelemetryHelper
} // namespace Optimizer
} // namespace Cutting
