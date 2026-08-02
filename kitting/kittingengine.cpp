#include "kittingengine.h"
#include "product/material_role_utils.h"
//#include "materials/registry/material_registry.h"
//#include "product/registry/bom_registry.h"
//#include "product/registry/material_role_registry.h"
//#include "product/registry/product_subtype_registry.h"
#include "product/registry/product_type_registry.h"

#include "kittingengine.h"
//#include "product/material_role_utils.h"
//#include "materials/registry/material_registry.h"
//#include "product/registry/bom_registry.h"
//#include "product/registry/material_role_registry.h"
//#include "product/registry/product_subtype_registry.h"
#include "product/registry/product_type_registry.h"
#include "naphalo/kitting_naphalo.h"

QVector<KittingInstruction> KittingEngine::expand(
    const Cutting::Plan::Request& req,
    const Cutting::Piece::PieceWithMaterial& pwm,
    const Cutting::Plan::CutPlan& plan)
{
    auto type = ProductTypeRegistry::instance().findById(req.productTypeId);

    if(!type) return {};

    if (type->code == "NP")
        return Kitting::Naphalo::expand(req, pwm, plan);

    if (type->code == "SR")
        //return Kitting::Savrolo::expand(req, pwm, plan);

    if (type->code == "ROL")
        //return Kitting::Roletta::expand(req, pwm, plan);

    return {};
}


