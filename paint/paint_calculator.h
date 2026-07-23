#pragma once

#include "paint/paint_plan.h"

class PaintCalculator {
public:

    static const QUuid CL_COMPOSITE_ID;

    static PaintPlan buildPlan();
};
