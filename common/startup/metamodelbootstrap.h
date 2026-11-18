#pragma once

#include <worksheetsummary/model/worksheetsummary.h>

namespace MetaModelBootstrap{
    inline void Init()
    {
        WorkSheetSummary::MetaInit();
        WorkSheetOrderInfo::MetaInit();
        WorkSheetMaterialInfo::MetaInit();
    }
}