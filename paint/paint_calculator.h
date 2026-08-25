#pragma once

#include "materials/model/material_master.h"
#include "paint/paint_plan.h"

class PaintCalculator {
public:
    static PaintPlan buildPlan();


    static void addComponent(PaintColorGroup& colorGroup,
                      const MaterialMaster* mat,
                      int pieceCount,
                      int length_mm,
                      double kgPerMeter,
                      const QUuid& requestId);

};
