#pragma once

#include "common/logger.h"
#include <QString>
#include <QUuid>

#include <product/registry/product_subtype_registry.h>
#include <product/registry/product_type_registry.h>

namespace SubtypeUtils {

//inline QString toProductVariantDisplayText(const QUuid& typeId, const QUuid& subtypeId)
//{
//    const bool typeMissing    = typeId.isNull();
//    const bool subtypeMissing = subtypeId.isNull();

//    const auto* type    = typeMissing    ? nullptr : ProductTypeRegistry::instance().findById(typeId);
//    const auto* subtype = subtypeMissing ? nullptr : ProductSubtypeRegistry::instance().findById(subtypeId);

//    // 🔹 Mindkettő hiányzik → régi CSV, nincs típus
//    if (!type && !subtype)
//        return "(ismeretlen/ismeretlen)";

//    // 🔹 Van típus, nincs altípus
//    if (type && !subtype)
//        return type->name + "/ismeretlen";

//    // 🔹 Nincs típus, van altípus
//    if (!type && subtype)
//        return "ismeretlen/" + subtype->name;

//    // 🔹 Mindkettő létezik
//    return type->name + "/" + subtype->name;
//}

inline QString toProductVariantDisplayText(const QUuid& typeId, const QUuid& subtypeId)
{
    //const bool typeMissing    = typeId.isNull();
    const bool subtypeMissing = subtypeId.isNull();

    //const auto* type    = typeMissing    ? nullptr : ProductTypeRegistry::instance().findById(typeId);
    const auto* subtype = subtypeMissing ? nullptr : ProductSubtypeRegistry::instance().findById(subtypeId);

    // 🔹 Mindkettő hiányzik → régi CSV, nincs típus
    //if (!type && !subtype)
    //    return "(ismeretlen/ismeretlen)";

    // 🔹 Van típus, nincs altípus
    if (!subtype)
        return "ismeretlen tip.";

    // 🔹 Nincs típus, van altípus
    //if (subtype)
        return subtype->name;

    // 🔹 Mindkettő létezik
    //return type->name + "/" + subtype->name;
}

} // namespace SubtypeUtils
