// piecewithmaterial.cpp

#include "piecewithmaterial.h"

namespace Cutting {
namespace Piece {


PieceWithMaterial::PieceWithMaterial()
    : info(), materialId()
{}

PieceWithMaterial::PieceWithMaterial(const PieceInfo& i, const QUuid& matId)
    : info(i), materialId(matId)
{}

bool PieceWithMaterial::operator==(const PieceWithMaterial& other) const {
    return info.length_mm == other.info.length_mm &&
           materialId == other.materialId;
}

} // endof namespace Piece
} // endof namespace Cutting
