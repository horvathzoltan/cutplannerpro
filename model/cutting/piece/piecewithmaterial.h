// model/cutting/piecewithmaterial.h

#pragma once

#include <QUuid>
#include "model/cutting/plan/request.h"
#include "pieceinfo.h"

/**
 * @brief Egy darab + anyagkapcsolat optimalizáláshoz
 */

namespace Cutting {
namespace Piece {

// Egy darab-anyag kapcsolat az optimalizáláshoz

class PieceWithMaterial
{
public:
    PieceInfo info;
    QUuid materialId = QUuid(); // Anyag azonosítója (UUID);

    Subtype subtype = Subtype::None; ///< Szerkezeti elem típusa (Alap, Rugós, Tetőteríti, stb.)
    HandlerSide side = HandlerSide::None; ///< kezelő/hajtómű oldala (bal/jobb/ismeretlen)

    PieceWithMaterial();
    PieceWithMaterial(const PieceInfo& i, const QUuid& matId);

    bool operator==(const PieceWithMaterial& other) const;
    QString displayText() const;
};

} //endof namespace Piece
} // endof namespace Cutting
