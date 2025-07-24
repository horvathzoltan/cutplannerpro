// piecewithmaterial.cpp

#include "piecewithmaterial.h"

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
