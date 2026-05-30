#pragma once

#include "model/leftoverstockentry.h"
#include "model/cutting/plan/cutplan.h"
#include "service/cutting/optimizer/optimizerconstants.h"
#include "selectedrod.h"

namespace Cutting {
namespace Optimizer {
namespace LeftoverLifecycle {

LeftoverStockEntry createPhysicalLeftover(
    const SelectedRod& rod,
    int remainingLength,
    int currentOpId,
    const QVector<Cutting::Plan::CutPlan>& resultPlans,
    bool& created);

} // namespace LeftoverLifecycle
} // namespace Optimizer
} // namespace Cutting
