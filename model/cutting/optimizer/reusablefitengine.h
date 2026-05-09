#pragma once

#include <optional>
#include <QVector>
#include "cuttypes.h"
#include "../piece/piecewithmaterial.h"
#include "../../leftoverstockentry.h"

class ReusableFitEngine {
public:

    /**
 * @brief Megkeresi, hogy a hulló készletből (reusableInventory) van-e olyan darab,
 *        amiből érdemes újra vágni.
 *
 * Mit csinál?
 * - Végignézi az összes hulló rudat (reusableInventory),
 * - Megnézi, hogy az adott anyagcsoport darabjai közül melyek férnek bele,
 * - Kiszámolja, mennyi darabot lehet belőle kivágni és mennyi hulladék marad,
 * - Kiválasztja a legjobb találatot (ahol a legtöbb darabot sikerül elhelyezni,
 *   és a legkevesebb a veszteség).
 *
 * Ha talál ilyet, visszaad egy ReusableCandidate-et, ami tartalmazza:
 * - melyik hulló rúd volt az,
 * - milyen darabokat sikerült belőle kivágni,
 * - mennyi hulladék maradt.
 *
 * Ha nem talál megfelelő hullót, akkor üres (std::nullopt) az eredmény.
 *
 * Röviden: ez a "hulló-vadász", ami megmondja,
 * hogy érdemes-e egy meglévő maradékból dolgozni, vagy sem.
 */

    static std::optional<ReusableCandidate>
    findBestReusableFit(const QVector<LeftoverStockEntry>& mergedView,
                        int globalCount,
                        const QVector<Cutting::Piece::PieceWithMaterial>& pieces,
                        QUuid materialId,
                        double kerf_mm,
                        const QSet<QUuid>& usedLeftoverEntryIds);
};
