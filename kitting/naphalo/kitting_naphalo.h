#pragma once


#include "kitting/model/kittinginstruction.h"
#include "kitting/naphalo/kitting_naphalo_cipzaras.h"
#include "kitting/naphalo/kitting_naphalo_sines.h"

#include <model/cutting/plan/cutplan.h>
#include <model/cutting/plan/request.h>

#include <model/cutting/piece/piecewithmaterial.h>

#include <product/registry/product_subtype_registry.h>
namespace Kitting{
namespace Naphalo{

inline QVector<KittingInstruction> expand(
    const Cutting::Plan::Request& req,
    const Cutting::Piece::PieceWithMaterial& pwm,
    const Cutting::Plan::CutPlan& plan)
{
    auto subtype = ProductSubtypeRegistry::instance().findById(req.productSubtypeId);

    if(!subtype) return {};

    if(subtype->code == "CIP"){
        return Cipzaras::expand(req,pwm, plan);
    }
    else if(subtype->code == "SIN"){
        return Sines::expand(req,pwm, plan);
    }
    else if(subtype->code == "BOW"){
        //return Bowdenes::expand(req,pwm, plan);
    }
    else {
        zInfo("Ismeretlen altípus:"+subtype->code);
    }

    return {};
}

} // end namespace Naphalo
}  // end namespace Calculation