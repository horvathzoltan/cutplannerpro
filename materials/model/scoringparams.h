#pragma once

#include "service/cutting/optimizer/optimizerconstants.h"
struct MaterialScoringParams {
    int scrap_mm;
    int goodLeftOver_Min_mm;
    int goodLeftOver_Max_mm;

    static MaterialScoringParams getDefault(){
        return {
            OptimizerConstants_2::SELEJT_THRESHOLD,
            OptimizerConstants_2::GOOD_LEFTOVER_MIN,
                OptimizerConstants_2::GOOD_LEFTOVER_MAX
        };
    }
};

