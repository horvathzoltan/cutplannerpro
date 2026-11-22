#pragma once

#include <worksheetsummary/model/worksheetsummary.h>

#include <meta/metaregistry.h>

namespace MetaModelBootstrap{
    inline void Init()
    {
        WorkSheetSummary::MetaInit();
        WorkSheetOrderInfo::MetaInit();
        WorkSheetMaterialInfo::MetaInit();
    }

    inline void Finalize(){
        MetaRegistry::Finalize();
    }

    inline void Seal(){
        MetaRegistry::Seal();
    }
}