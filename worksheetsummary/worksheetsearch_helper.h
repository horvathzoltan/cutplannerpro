#pragma once

#include <worksheetsummary/model/worksheetorderinfo.h>
#include <worksheetsummary/model/worksheetsummary.h>
#include <product/registry/product_subtype_registry.h>
#include <product/registry/product_type_registry.h>

namespace WorkSheetSearchHelper
{

inline bool matches(
    const WorkSheetOrderInfo& order,
    const QString& searchText)
{
    QString txt =
        searchText.trimmed();

    if (txt.isEmpty())
    {
        return false;
    }

    auto* type =
        ProductTypeRegistry::instance()
            .findById(order.productTypeId);

    auto* subtype =
        ProductSubtypeRegistry::instance()
            .findById(order.productSubtypeId);

    QString typeName =
        type
            ? type->name
            : QString();

    QString subtypeName =
        subtype
            ? subtype->name
            : QString();

    return
        order.externalReference.contains(
            txt,
            Qt::CaseInsensitive)
        ||
        order.ownerName.contains(
            txt,
            Qt::CaseInsensitive)
        ||
        order.requiredColor.code().contains(
            txt,
            Qt::CaseInsensitive)
        ||
        typeName.contains(
            txt,
            Qt::CaseInsensitive)
        ||
        subtypeName.contains(
            txt,
            Qt::CaseInsensitive);
}


inline bool matches(
    const WorkSheetSummary& summary,
    const QString& searchText)
{
    for (const auto& order : summary.orders)
    {
        if (matches(order, searchText))
        {
            return true;
        }
    }

    return false;
}

} // endof namespace WorkSheetSearchHelper
