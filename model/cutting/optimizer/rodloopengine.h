#pragma once

#include <QVector>
#include "../piece/piecewithmaterial.h"
#include "selectedrod.h"
#include "../cuttingmachine.h"

namespace Cutting {
namespace Optimizer {

class OptimizerModel; // forward

class RodLoopEngine {
public:

    enum class RodStepResult {
        ContinueSameRod,   // belső while következő iteráció
        StartNewRod,       // külső while új rúd
        StartNewStockRod,  // külső while új stock rúd
        StopRod            // finalizeRod + kilépés a rod-loopból
    };

    struct RodStepResultModel {
        RodStepResult rodStepResult;
        QUuid materialId; // mi az anyag amit valójában a materiialgroup szerint tudunk vágniu
    };

    static RodStepResultModel step(
        QVector<Cutting::Piece::PieceWithMaterial>& groupVec,
        int& remainingLength,
        int& dpLimit,
        const SelectedRod& rod,
        const CuttingMachine& machine,
        int currentOpId,
        int rodId,
        double kerf_mm,
        OptimizerModel& model);
};

} // namespace Optimizer
} // namespace Cutting
