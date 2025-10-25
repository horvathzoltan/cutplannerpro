#pragma once
#include <QVector>
#include <numeric>
#include "model/cutting/piece/piecewithmaterial.h"
#include "service/cutting/optimizer/optimizerconstants.h"

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
 * @brief Pontszámítást végez egy adott darabkombinációra.
 *
 * A függvény célja, hogy összehasonlíthatóvá tegye a különböző vágási lehetőségeket,
 * és kiválassza közülük a legjobbat. A magasabb pontszám a kedvezőbb megoldást jelzi.
 *
 * A számítás szempontjai:
 * - @b Darabszám: minden kivágott darab +100 pontot ér.
 * - @b Hulladék: a keletkező hulladék hossza levonásra kerül a pontszámból.
 * - @b Teljes elfogyasztás: ha waste == 0, akkor +800 bónusz jár.
 * - @b Jó leftover: ha a maradék a GOOD_LEFTOVER_MIN és GOOD_LEFTOVER_MAX közé esik,
 *   akkor +300 bónusz jár (pszichológiailag „jó érzésű” maradék).
 * - @b Selejt leftover: ha a maradék 0 < leftover < SELEJT_THRESHOLD,
 *   akkor −300 büntetés jár (bosszantóan kicsi hulladék).
 * - @b Pontos egy darabos illeszkedés: ha egyetlen darabbal pontosan elfogy a rúd,
 *   további +200 bónusz jár.
 * - @b Túl nagy leftover: ha a maradék > GOOD_LEFTOVER_MAX,
 *   akkor −100 büntetés jár (valószínűleg kiadhatna még egy darabot, de nem ad).
 *
 * @param pieceCount     A kivágott darabok száma.
 * @param waste          A vágás után keletkező hulladék hossza (mm).
 * @param leftoverLength A rúd fennmaradó hossza (mm).
 * @return int           A kombináció pontszáma; minél nagyobb, annál jobb.
 *
 * @note Ez a pontozás nem pusztán matematikai hatékonyságot tükröz,
 *       hanem a műhelypszichológiát is: preferálja a teljesen elfogyasztott rudakat
 *       és a jól használható maradékokat, miközben bünteti a bosszantóan kicsi
 *       vagy a „túl nagy, de kihasználatlan” hulladékokat.
 */
inline int calcScore(int pieceCount, int waste, int leftoverLength) {
    int score = 0;
    score += pieceCount * 100;
    score -= waste;

    if (waste == 0) score += 800;

    // Jó leftover tartomány
    if (leftoverLength >= OptimizerConstants::GOOD_LEFTOVER_MIN &&
        leftoverLength <= OptimizerConstants::GOOD_LEFTOVER_MAX) {
        score += 300;
    }

    // Selejt leftover
    if (leftoverLength > 0 && leftoverLength < OptimizerConstants::SELEJT_THRESHOLD) {
        score -= 300;
    }

    // Pontos egy darabos illeszkedés
    if (leftoverLength == 0 && pieceCount == 1) {
        score += 200;
    }

    // Túl nagy leftover
    if (leftoverLength > OptimizerConstants::GOOD_LEFTOVER_MAX) {
        score -= 100;
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


} // namespace OptimizerUtils
