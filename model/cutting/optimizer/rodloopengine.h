#pragma once

#include <QVector>
#include "../piece/piecewithmaterial.h"
#include "selectedrod.h"
#include "../cuttingmachine.h"

namespace Cutting {
namespace Optimizer {

class OptimizerModel; // forward

enum class RodStepResult {
    ContinueSameRod,   // belső while következő iteráció
    StartNewRod,       // külső while új rúd
    StopRod            // finalizeRod + kilépés a rod-loopból
};

class RodLoopEngine {
public:
    static RodStepResult step(
        QVector<Cutting::Piece::PieceWithMaterial>& groupVec,
        int& remainingLength,
        int& remainingLength2,
        const SelectedRod& rod,
        const CuttingMachine& machine,
        int currentOpId,
        int rodId,
        double kerf_mm,
        OptimizerModel& model);
};

} // namespace Optimizer
} // namespace Cutting
