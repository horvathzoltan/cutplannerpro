#pragma once

#include "service/cutting/optimizer/optimizerconstants.h"
struct MaterialTrimmingParams {
    int frontTrim_mm;
    int backTrim_mm;
    int minLeftOver_mm;

    static MaterialTrimmingParams getDefault() {
        return {
            OptimizerConstants_2::END_TRIM_MM, // frontTrim_mm
            OptimizerConstants_2::END_TRIM_MM, // backTrim_mm
            OptimizerConstants_2::MINIMUM_HULLO_MM  // minLeftOver_mm
        };
    }
};
