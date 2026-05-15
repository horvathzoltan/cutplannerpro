#pragma once
#include "cutplan_input_summary.h"
#include "cutplan_output_summary.h"

#include <QDate>

#include <model/cutting/optimizer/optimizermodel.h>


// A CutPlanSummary NEM gépenkénti — ez GLOBÁLIS összefoglaló
struct CutPlanSummary {
    CutPlanInputSummary  input;   // vágás előtti állapot
    CutPlanOutputSummary output;  // vágás utáni állapot
    FitEngine::FitEngineTelemetry fitTelemetry;

    QString planIdStr;


    QString OptimizerSummary() const
    {
        long total = fitTelemetry.totalElapsed_us;
        auto pct = [&](long v){ return total > 0 ? (100.0 * v / total) : 0.0; };

        QString base = QString(
                   "🔧 Iterációk száma: %1\n"
                   "🔧 FitEngine hívások: %2\n"
                   "\n"
                   "🔍 Algoritmus-megoszlás:\n"
                   "    FullFit:     %3× → %4 találat | %5 µs | %6%\n"
                   "    BruteForce:  %7× → %8 találat | %9 µs | %10%\n"
                   "    DPPlain:     %11× → %12 találat | %13 µs | %14%\n"
                   "    DPScoring:   %15× → %16 találat | %17 µs | %18%\n"
                           "    Greedy:      %19× → %20 találat | %21 µs | %22%\n")
            .arg(fitTelemetry.calls)
            .arg(fitTelemetry.calls)

            .arg(fitTelemetry.fullFit)
            .arg(fitTelemetry.fullFitFound)
            .arg(fitTelemetry.totalFullFitElapsed_us)
            .arg(QString::number(pct(fitTelemetry.totalFullFitElapsed_us), 'f', 1))

            .arg(fitTelemetry.bruteForce)
            .arg(fitTelemetry.bruteForceFound)
            .arg(fitTelemetry.totalBFElapsed_us)
            .arg(QString::number(pct(fitTelemetry.totalBFElapsed_us), 'f', 1))

            .arg(fitTelemetry.dpPlain)
            .arg(fitTelemetry.dpPlainFound)
            .arg(fitTelemetry.totalDPPlainElapsed_us)
            .arg(QString::number(pct(fitTelemetry.totalDPPlainElapsed_us), 'f', 1))

            .arg(fitTelemetry.dpScoring)
            .arg(fitTelemetry.dpScoringFound)
            .arg(fitTelemetry.totalDPScoringElapsed_us)
            .arg(QString::number(pct(fitTelemetry.totalDPScoringElapsed_us), 'f', 1))

            .arg(fitTelemetry.greedy)
            .arg(fitTelemetry.greedyFound)
            .arg(fitTelemetry.totalGreedyElapsed_us)
            .arg(QString::number(pct(fitTelemetry.totalGreedyElapsed_us), 'f', 1));

        QString leftover =
            QString(
                "\n♻️ Leftover-hasznosulás algoritmusonként:\n"
                "    FullFit:     jó: %1 db (%2 mm), selejt: %3 db, túl nagy: %4 db\n"
                "    BruteForce:  jó: %5 db (%6 mm), selejt: %7 db, túl nagy: %8 db\n"
                "    DPPlain:     jó: %9 db (%10 mm), selejt: %11 db, túl nagy: %12 db\n"
                "    DPScoring:   jó: %13 db (%14 mm), selejt: %15 db, túl nagy: %16 db\n"
                "    Greedy:      jó: %17 db (%18 mm), selejt: %19 db, túl nagy: %20 db\n")
                .arg(fitTelemetry.fullFit_goodCount)
                .arg(fitTelemetry.fullFit_goodTotal)
                .arg(fitTelemetry.fullFit_badCount)
                .arg(fitTelemetry.fullFit_oversizeCount)

                .arg(fitTelemetry.bruteForce_goodCount)
                .arg(fitTelemetry.bruteForce_goodTotal)
                .arg(fitTelemetry.bruteForce_badCount)
                .arg(fitTelemetry.bruteForce_oversizeCount)

                .arg(fitTelemetry.dpPlain_goodCount)
                .arg(fitTelemetry.dpPlain_goodTotal)
                .arg(fitTelemetry.dpPlain_badCount)
                .arg(fitTelemetry.dpPlain_oversizeCount)

                .arg(fitTelemetry.dpScoring_goodCount)
                .arg(fitTelemetry.dpScoring_goodTotal)
                .arg(fitTelemetry.dpScoring_badCount)
                .arg(fitTelemetry.dpScoring_oversizeCount)

                .arg(fitTelemetry.greedy_goodCount)
                .arg(fitTelemetry.greedy_goodTotal)
                .arg(fitTelemetry.greedy_badCount)
                .arg(fitTelemetry.greedy_oversizeCount);

        auto champion = [&](long a, long b, long c, long d, long e) {
            QMap<long, QString> map;
            map[a] = "FullFit";
            map[b] = "BruteForce";
            map[c] = "DPPlain";
            map[d] = "DPScoring";
            map[e] = "Greedy";
            long best = std::max({a,b,c,d,e});
            return map[best];
        };

        QString champs =
            QString(
                "\n🏆 Bajnokok:\n"
                "    • Szálmegevő bajnok: %1\n"
                "    • Legjobban illesztő bajnok: %2\n"
                "    • Leftover-felhasználó bajnok: %3\n"
                "    • Selejtgyártó anti-bajnok: %4\n")
                .arg(champion(
                    fitTelemetry.fullFitFound,
                    fitTelemetry.bruteForceFound,
                    fitTelemetry.dpPlainFound,
                    fitTelemetry.dpScoringFound,
                    fitTelemetry.greedyFound))

                .arg(champion(
                    fitTelemetry.fullFit_goodCount,
                    fitTelemetry.bruteForce_goodCount,
                    fitTelemetry.dpPlain_goodCount,
                    fitTelemetry.dpScoring_goodCount,
                    fitTelemetry.greedy_goodCount))

                .arg(champion(
                    fitTelemetry.fullFit_goodTotal,
                    fitTelemetry.bruteForce_goodTotal,
                    fitTelemetry.dpPlain_goodTotal,
                    fitTelemetry.dpScoring_goodTotal,
                    fitTelemetry.greedy_goodTotal))

                .arg(champion(
                    fitTelemetry.fullFit_badCount,
                    fitTelemetry.bruteForce_badCount,
                    fitTelemetry.dpPlain_badCount,
                    fitTelemetry.dpScoring_badCount,
                    fitTelemetry.greedy_badCount));

        QString difficulty =
            QString(
                "\n📈 Algoritmus nehézségi profil:\n"
                "    • Könnyű esetek (FullFit + Greedy): %1 db\n"
                "    • Közepes esetek (DPPlain): %2 db\n"
                "    • Nehéz esetek (DPScoring): %3 db\n"
                "    • Extrém esetek (BruteForce): %4 db\n"
                "\n"
                "    • Fallback-lépések száma:\n"
                "         FullFit → Greedy: %5 db\n"
                "         Greedy → DPPlain: %6 db\n"
                "         DPPlain → DPScoring: %7 db\n"
                "         DPScoring → BruteForce: %8 db\n")

                // könnyű esetek
                .arg(fitTelemetry.fullFit + fitTelemetry.greedy)

                // közepes
                .arg(fitTelemetry.dpPlain)

                // nehéz
                .arg(fitTelemetry.dpScoring)

                // extrém
                .arg(fitTelemetry.bruteForce)

                // fallback láncok
                .arg(fitTelemetry.greedy)        // FullFit → Greedy
                .arg(fitTelemetry.dpPlain)       // Greedy → DPPlain
                .arg(fitTelemetry.dpScoring)     // DPPlain → DPScoring
                .arg(fitTelemetry.bruteForce);   // DPScoring → BruteForce


        QString dpProfile =
            QString(
                "\n📐 DP-hatékonysági profil:\n"
                "    • DPPlain futások: %1 db\n"
                "         - Átlagos DP-limit: %2\n"
                "         - Átlagos kitöltött cellák: %3\n"
                "         - Átlagos lánchossz: %4\n"
                "\n"
                "    • DPScoring futások: %5 db\n"
                "         - Átlagos DP-limit: %6\n"
                "         - Átlagos kitöltött cellák: %7\n"
                "         - Átlagos lánchossz: %8\n"
                "\n"
                "    • DPScoring előnye DPPlain-hez képest:\n"
                "         - Több találat: %9 db\n"
                "         - Kevesebb selejt leftover: %10 db\n"
                "         - Jobb leftover összérték: %11 mm\n")

                // DPPlain
                .arg(fitTelemetry.dpPlain)
                .arg(fitTelemetry.dpPlain > 0 ? fitTelemetry.dp_totalLimit / fitTelemetry.dpPlain : 0)
                .arg(fitTelemetry.dpPlain > 0 ? fitTelemetry.dp_totalFilledCells / fitTelemetry.dpPlain : 0)
                .arg(fitTelemetry.dpPlain > 0 ? fitTelemetry.dp_totalChainLength / fitTelemetry.dpPlain : 0)

                // DPScoring
                .arg(fitTelemetry.dpScoring)
                .arg(fitTelemetry.dpScoring > 0 ? fitTelemetry.dp_totalLimit / fitTelemetry.dpScoring : 0)
                .arg(fitTelemetry.dpScoring > 0 ? fitTelemetry.dp_totalFilledCells / fitTelemetry.dpScoring : 0)
                .arg(fitTelemetry.dpScoring > 0 ? fitTelemetry.dp_totalChainLength / fitTelemetry.dpScoring : 0)

                // DPScoring előnyök
                .arg(fitTelemetry.dpScoringFound - fitTelemetry.dpPlainFound)
                .arg(fitTelemetry.dpPlain_badCount - fitTelemetry.dpScoring_badCount)
                .arg(fitTelemetry.dpScoring_goodTotal - fitTelemetry.dpPlain_goodTotal);

        QString greedyProfile =
            QString(
                "\n🟩 Greedy-minőség profil:\n"
                "    • Greedy futások: %1 db\n"
                "         - Jó leftover: %2 db (%3 mm)\n"
                "         - Selejt leftover: %4 db\n"
                "         - Túl nagy leftover: %5 db\n"
                "\n"
                "    • Greedy minőségi mutatók:\n"
                "         - Jó leftover arány: %6%\n"
                "         - Selejt arány: %7%\n"
                "         - Túl nagy leftover arány: %8%\n"
                "\n"
                "    • Greedy stabilitás:\n"
                "         - Átlagos picks: %9\n"
                "         - Átlagos rendezett lista méret: %10\n"
                "         - Greedy minőség-index: %11\n")

                // futások
                .arg(fitTelemetry.greedy)

                // leftover statok
                .arg(fitTelemetry.greedy_goodCount)
                .arg(fitTelemetry.greedy_goodTotal)
                .arg(fitTelemetry.greedy_badCount)
                .arg(fitTelemetry.greedy_oversizeCount)

                // arányok
                .arg(fitTelemetry.greedy > 0 ? (100.0 * fitTelemetry.greedy_goodCount / fitTelemetry.greedy) : 0.0, 0, 'f', 1)
                .arg(fitTelemetry.greedy > 0 ? (100.0 * fitTelemetry.greedy_badCount / fitTelemetry.greedy) : 0.0, 0, 'f', 1)
                .arg(fitTelemetry.greedy > 0 ? (100.0 * fitTelemetry.greedy_oversizeCount / fitTelemetry.greedy) : 0.0, 0, 'f', 1)

                // stabilitás
                .arg(fitTelemetry.greedy > 0 ? fitTelemetry.greedy_totalPicks / fitTelemetry.greedy : 0)
                .arg(fitTelemetry.greedy > 0 ? fitTelemetry.greedy_totalSortedSize / fitTelemetry.greedy : 0)

                // minőség-index
                .arg(fitTelemetry.greedy > 0
                         ? (fitTelemetry.greedy_goodTotal
                            - fitTelemetry.greedy_badTotal
                            - fitTelemetry.greedy_oversizeTotal) / fitTelemetry.greedy
                         : 0);

        QString bruteProfile =
            QString(
                "\n🟥 BruteForce-minőség profil:\n"
                "    • BruteForce futások: %1 db\n"
                "         - Jó leftover: %2 db (%3 mm)\n"
                "         - Selejt leftover: %4 db\n"
                "         - Túl nagy leftover: %5 db\n"
                "\n"
                "    • BruteForce minőségi mutatók:\n"
                "         - Jó leftover arány: %6%\n"
                "         - Selejt arány: %7%\n"
                "         - Túl nagy leftover arány: %8%\n"
                "\n"
                "    • BruteForce keresési statisztikák:\n"
                "         - Átlagos kombinációszám: %9\n"
                "         - Átlagos értékelt kombináció: %10\n"
                "         - Átlagos kihagyott kombináció: %11\n"
                "         - Átlagos futásidő: %12 ms\n"
                "\n"
                "    • BruteForce stabilitás:\n"
                "         - Jó leftover / futás: %13\n"
                "         - Selejt leftover / futás: %14\n"
                "         - Minőség-index: %15\n")

                // futások
                .arg(fitTelemetry.bruteForce)

                // leftover statok
                .arg(fitTelemetry.bruteForce_goodCount)
                .arg(fitTelemetry.bruteForce_goodTotal)
                .arg(fitTelemetry.bruteForce_badCount)
                .arg(fitTelemetry.bruteForce_oversizeCount)

                // arányok
                .arg(fitTelemetry.bruteForce > 0 ? (100.0 * fitTelemetry.bruteForce_goodCount / fitTelemetry.bruteForce) : 0.0, 0, 'f', 1)
                .arg(fitTelemetry.bruteForce > 0 ? (100.0 * fitTelemetry.bruteForce_badCount / fitTelemetry.bruteForce) : 0.0, 0, 'f', 1)
                .arg(fitTelemetry.bruteForce > 0 ? (100.0 * fitTelemetry.bruteForce_oversizeCount / fitTelemetry.bruteForce) : 0.0, 0, 'f', 1)

                // keresési statisztikák
                .arg(fitTelemetry.bruteForce > 0 ? fitTelemetry.bf_totalCombos / fitTelemetry.bruteForce : 0)
                .arg(fitTelemetry.bruteForce > 0 ? fitTelemetry.bf_totalEvaluated / fitTelemetry.bruteForce : 0)
                .arg(fitTelemetry.bruteForce > 0 ? fitTelemetry.bf_totalSkipped / fitTelemetry.bruteForce : 0)
                .arg(fitTelemetry.bruteForce > 0 ? (fitTelemetry.totalBFElapsed_us / 1000.0 / fitTelemetry.bruteForce) : 0.0, 0, 'f', 2)

                // stabilitás
                .arg(fitTelemetry.bruteForce > 0 ? double(fitTelemetry.bruteForce_goodCount) / fitTelemetry.bruteForce : 0.0, 0, 'f', 2)
                .arg(fitTelemetry.bruteForce > 0 ? double(fitTelemetry.bruteForce_badCount) / fitTelemetry.bruteForce : 0.0, 0, 'f', 2)
                .arg(fitTelemetry.bruteForce > 0
                         ? (fitTelemetry.bruteForce_goodTotal
                            - fitTelemetry.bruteForce_badTotal
                            - fitTelemetry.bruteForce_oversizeTotal) / fitTelemetry.bruteForce
                         : 0);

        QString fallback =
            QString(
                "\n🔻 Fallback-lánc\n"
                "\n"
                "    FullFit (%1)  - determinista, teljes illesztést kereső gyorsvizsgálat\n"
                "        │\n"
                "        ├──→ Greedy (%2) - lokálisan optimális választásokat halmozó heurisztikus illesztő\n"
                "        │        │\n"
                "        │        ├──→ DPPlain (%3) - dinamikus programozás alapú, súlyozatlan kapacitás‑optimalizáló\n"
                "        │        │        │\n"
                "        │        │        ├──→ DPScoring (%4) - dinamikus programozás kiterjesztett értékeléssel és többdimenziós pontozással\n"
                "        │        │        │        │\n"
                "        │        │        │        └──→ BruteForce (%5) - teljes keresési tér bejárása, garantált optimum, exponenciális költséggel\n"
                "        │        │        │\n"
                "        │        │        └── visszatérés\n"
                "        │        └── visszatérés\n"
                "        └── visszatérés\n")

                .arg(fitTelemetry.fullFit)
                .arg(fitTelemetry.greedy)
                .arg(fitTelemetry.dpPlain)
                .arg(fitTelemetry.dpScoring)
                .arg(fitTelemetry.bruteForce);


        QString result = base;

        // leftover blokk mindig kell
        result += leftover;

        // bajnokok mindig kellenek
        result += champs;

        // nehézségi profil mindig kell
        result += difficulty;

        // DPPlain / DPScoring profil csak akkor, ha van DP futás
        if (fitTelemetry.dpPlain > 0 || fitTelemetry.dpScoring > 0)
            result += dpProfile;

        // Greedy profil csak akkor, ha volt Greedy futás
        if (fitTelemetry.greedy > 0)
            result += greedyProfile;

        // BruteForce profil csak akkor, ha volt BruteForce futás
        if (fitTelemetry.bruteForce > 0)
            result += bruteProfile;

        // fallback-lánc mindig kell
        result += fallback;

        return result;
    }

    QString toText() const {
        QString dateStr = QDateTime::currentDateTime().toString("yyyy.MM.dd HH:mm");

        QStringList lines;

        lines << QString("📥 Gyártási összefoglaló (globális)");
        lines << QString("CutPlan: %1").arg(planIdStr);
        lines << QString("📅 Dátum: %1").arg(dateStr);
        lines <<  "────────────────────────────────";
        lines << input.toText();
        lines << "────────────────────────────────";
        lines << OptimizerSummary();
        lines << "────────────────────────────────";
        lines << output.toText();

        return lines.join("\n");
    }


};
