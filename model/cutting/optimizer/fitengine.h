#pragma once

#include <QVector>
#include "../piece/piecewithmaterial.h"

class FitEngine {
public:

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

    static QVector<Cutting::Piece::PieceWithMaterial>
    findBestFit(const QVector<Cutting::Piece::PieceWithMaterial>& available,
                int lengthLimit,
                double kerf_mm);

};
