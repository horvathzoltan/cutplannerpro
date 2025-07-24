// model/cutting/piecewithmaterial.h

#pragma once

#include <QUuid>
#include "model/pieceinfo.h"

/**
 * @brief Egy darab + anyagkapcsolat optimalizáláshoz
 */
class PieceWithMaterial
{
public:
    PieceInfo info;
    QUuid materialId = QUuid(); // Anyag azonosítója (UUID);

    PieceWithMaterial();
    PieceWithMaterial(const PieceInfo& i, const QUuid& matId);

    bool operator==(const PieceWithMaterial& other) const;
};

