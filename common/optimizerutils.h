#pragma once
#include <QVector>
#include <numeric>
#include "model/cutting/piece/piecewithmaterial.h"

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

// Score számítása
/**
 * @brief Egy kombináció pontszámát számolja ki.
 *
 * A pontszám két dologból áll:
 * - hány darabot sikerült elhelyezni (ez a legfontosabb),
 * - mennyi hulladék maradt (ez a második szempont).
 *
 * A darabszámot 1000-rel megszorozzuk, hogy mindig fontosabb legyen,
 * mint a hulladék. Így +1 darab kb. 1000 mm hulladékot is "megér".
 *
 * Példa:
 * - 3 darab, 50 mm hulladék → score = 3*1000 - 50 = 2950
 * - 4 darab, 200 mm hulladék → score = 4*1000 - 200 = 3800
 *   → hiába több a hulladék, a +1 darab miatt ez a jobb.
 *
 * Röviden: ez a "pontozóbíró", ami eldönti,
 * melyik kombináció a nyerő. Ha egyszer lesznek árak/forintok,
 * akkor ezt a képletet át lehet alakítani valódi költségszámításra.
 */

inline int calcScore(int pieceCount, int waste, int weight = 1000) {
    return pieceCount * weight - waste;
}

} // namespace OptimizerUtils
