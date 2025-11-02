#pragma once
#include <QVector>
#include <numeric>
#include <model/registries/materialregistry.h>
#include <model/leftoverstockentry.h>
#include "model/cutting/piece/piecewithmaterial.h"
#include "model/cutting/plan/cutplan.h"
#include "service/cutting/optimizer/optimizerconstants.h"
#include "model/cutting/cuttingmachine.h"

namespace OptimizerUtils {
                                                          // Darabhosszak összege
inline int sumLengths(const QVector<Cutting::Piece::PieceWithMaterial>& combo) {
    return std::accumulate(combo.begin(), combo.end(), 0,
                           [](int sum, const auto& pwm) { return sum + pwm.info.length_mm; });
}

// Kerf veszteség számítása (darabszám × kerf, kerekítve)
inline int roundKerfLoss(int pieceCount, double kerf_mm) {
    return static_cast<int>(std::lround(pieceCount * kerf_mm));
}

// Hulladék számítása (negatív érték ne legyen)
inline int computeWasteInt(int selectedLength_mm, int used_mm) {
    const int w = selectedLength_mm - used_mm;
    return w < 0 ? 0 : w;
}

/**
 * @brief Kombinációs pontszám számítása.
 *
 * A függvény célja, hogy összehasonlíthatóvá tegye a különböző vágási lehetőségeket,
 * és kiválassza közülük a legjobbat. A magasabb pontszám a kedvezőbb megoldást jelzi.
 *
 * Szempontok:
 * - Darabszám: minden kivágott darab +100 pont.
 * - Hulladék: a keletkező hulladék hossza levonásra kerül.
 * - Teljes elfogyasztás: ha waste == 0, akkor +800 bónusz.
 * - Pontos egy darabos illeszkedés: ha leftover == 0 és pieceCount == 1, további +200 bónusz.
 * - Jó leftover: ha leftover a GOOD_LEFTOVER_MIN és GOOD_LEFTOVER_MAX közé esik, +300 bónusz.
 * - Selejt leftover: ha 0 < leftover < SELEJT_THRESHOLD, akkor −300 büntetés.
 * - Túl nagy leftover: ha leftover > GOOD_LEFTOVER_MAX, akkor dinamikus büntetés
 *   (−100 alap + mérettel arányos levonás).
 *
 * @param pieceCount     A kivágott darabok száma.
 * @param waste          A vágás után keletkező hulladék hossza (mm).
 * @param leftoverLength A rúd fennmaradó hossza (mm).
 * @return int           A kombináció pontszáma; minél nagyobb, annál jobb.
 *
 * @note A pontozás nem pusztán matematikai hatékonyságot tükröz,
 *       hanem a műhelypszichológiát is: preferálja a teljesen elfogyasztott rudakat
 *       és a jól használható maradékokat, miközben bünteti a bosszantóan kicsi
 *       vagy a túl nagy, kihasználatlan maradékokat.
 */

inline int calcScore(int pieceCount, int waste, int leftoverLength) {
    int score = 0;

    // 🎯 Alap: darabszám preferálása
    score += pieceCount * 100;

    // 🧹 Hulladék levonása
    score -= waste;

    // 🥇 Teljes elfogyasztás – pszichológiai bónusz
    if (waste == 0) {
        score += 800;
    }

    // 🎯 Pontos egy darabos illeszkedés – külön jutalom
    if (leftoverLength == 0 && pieceCount == 1) {
        score += 200;
    }

    // 😊 Jó leftover tartomány – „jó érzésű” maradék
    if (leftoverLength >= OptimizerConstants::GOOD_LEFTOVER_MIN &&
        leftoverLength <= OptimizerConstants::GOOD_LEFTOVER_MAX) {
        score += 300;
    }

    // 😬 Selejt leftover – erősebb büntetés
    if (leftoverLength > 0 && leftoverLength < OptimizerConstants::SELEJT_THRESHOLD) {
        score -= 300; // visszaállítva az eredeti szigorra
    }

    // 🧱 Túl nagy leftover – dinamikus büntetés
    if (leftoverLength > OptimizerConstants::GOOD_LEFTOVER_MAX) {
        // Alapbüntetés −100, de skálázva a mérettel
        int oversize = leftoverLength - OptimizerConstants::GOOD_LEFTOVER_MAX;
        score -= 100 + oversize / 100;
        // pl. 900 mm leftover → −110, 1500 mm leftover → −1150
    }

    return score;
}



/**
 * @brief Megkeresi azt az egyetlen darabot, amelyik a legjobban illeszkedik a maradék hosszba.
 *
 * A pontozás a calcScore() alapján történik, tehát preferálja a teljes elfogyasztást
 * és a jó leftover méreteket. Ha nincs olyan darab, ami beleférne, std::nullopt‑ot ad vissza.
 *
 * @param available     Az elérhető darabok listája.
 * @param lengthLimit   A rúd maradék hossza.
 * @param kerf_mm       A vágási veszteség (mm).
 * @return std::optional<Cutting::Piece::PieceWithMaterial>
 */
inline std::optional<Cutting::Piece::PieceWithMaterial>
findSingleBestPiece(const QVector<Cutting::Piece::PieceWithMaterial>& available,
                    int lengthLimit,
                    double kerf_mm)
{
    std::optional<Cutting::Piece::PieceWithMaterial> best;
    int bestScore = std::numeric_limits<int>::min();

    for (const auto& piece : available) {
        int used = piece.info.length_mm + OptimizerUtils::roundKerfLoss(1, kerf_mm);
        if (used > lengthLimit) continue;

        int waste = OptimizerUtils::computeWasteInt(lengthLimit, used);
        int leftoverLength = lengthLimit - used;
        int score = OptimizerUtils::calcScore(1, waste, leftoverLength);

        if (score > bestScore) {
            bestScore = score;
            best = piece;
        }
    }
    return best;
}

/**
 * @brief Megkeresi azt az egyetlen darabot, amelyik pontosan elfogyasztja a megadott hosszt.
 *
 * Ha találunk olyan darabot, amelyik + kerf pontosan kitölti a rúd hosszát (waste == 0),
 * akkor azt visszaadjuk. Ellenkező esetben std::nullopt.
 *
 * @param available     Az elérhető darabok listája.
 * @param lengthLimit   A rúd teljes hossza.
 * @param kerf_mm       A vágási veszteség (mm).
 * @return std::optional<Cutting::Piece::PieceWithMaterial>
 */
inline std::optional<Cutting::Piece::PieceWithMaterial>
findSingleExactFit(const QVector<Cutting::Piece::PieceWithMaterial>& available,
                   int lengthLimit,
                   double kerf_mm)
{
    for (const auto& piece : available) {
        int used = piece.info.length_mm + OptimizerUtils::roundKerfLoss(1, kerf_mm);
        if (used == lengthLimit) {
            return piece; // pontos illeszkedés
        }
    }
    return std::nullopt;
}

inline QString formatLeftoverEvent(const LeftoverStockEntry& entry, QString rodId) {
    return QString("♻️ Hulló létrehozva: %1 (entryId=%2, rodId=%3)")
        .arg(entry.barcode)
        .arg(entry.entryId.toString())
        .arg(rodId);
}


inline QString formatCutPlanEvent(const Cutting::Plan::CutPlan& plan,
                                  const CuttingMachine& machine) {
    // Forrás azonosító
    QString sourceLabel;
    if (plan.source == Cutting::Plan::Source::Reusable) {
        sourceLabel = QString("Hulló: %1").arg(plan.sourceBarcode);
    } else {
        const MaterialMaster* mat = MaterialRegistry::instance().findById(plan.materialId);
        sourceLabel = mat ? QString("Anyag: %1").arg(mat->name)
                          : QString("Anyag: ? (%1)").arg(plan.materialId.toString());
    }

    // Darablista
    QMap<int,int> pieceCount; // hossz → darabszám
    for (const auto& pw : plan.piecesWithMaterial) {
        pieceCount[pw.info.length_mm] += 1;
    }
    QStringList pieceList;
    for (auto it = pieceCount.begin(); it != pieceCount.end(); ++it) {
        pieceList << QString("%1×%2 mm").arg(it.value()).arg(it.key());
    }

    return QString("🪚 CutPlan #%1 → %2, Rod=%3, gép=%4, kerf=%5 mm, darabok: %6, hulladék=%7 mm")
        .arg(plan.planNumber)
        .arg(sourceLabel)
        .arg(plan.rodId)
        .arg(machine.name)
        .arg(QString::number(plan.kerfUsed_mm, 'f', 1)) // 🔧 pontos formázás
        .arg(pieceList.join(", "))
        .arg(plan.waste);
}

} // namespace OptimizerUtils
