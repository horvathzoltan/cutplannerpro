// piecewithmaterial.cpp

#include "piecewithmaterial.h"

#include <model/registries/cuttingplanrequestregistry.h>

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


QString PieceWithMaterial::displayText() const{
    Cutting::Plan::Request *p = CuttingPlanRequestRegistry::instance().findById(info.requestId);
    if (p == nullptr)
        return "";

    return p->displayText();
}
} // endof namespace Piece
} // endof namespace Cutting
