// model/cutting/piecewithmaterial.h

#pragma once

#include <QUuid>
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

    PieceWithMaterial();
    PieceWithMaterial(const PieceInfo& i, const QUuid& matId);

    bool operator==(const PieceWithMaterial& other) const;
};

} //endof namespace Piece
} // endof namespace Cutting
