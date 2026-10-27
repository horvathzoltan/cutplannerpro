#include "worksheetsummary_searchengine.h"
#include "view/columnindexes/worksheetorderrow_columns.h"

#include <product/registry/product_subtype_registry.h>
#include <product/registry/product_type_registry.h>

void WorkSheetSummarySearchEngine::searchOrder(
    const WorkSheetOrderInfo& order,
    const QString& text,
    SearchResult& result)
{
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

    if (order.externalReference.contains(text,Qt::CaseInsensitive))
    {
        result.hits.push_back({SearchTarget::Order,order.id,toColumnIndex(WorkSheetOrderColumn::Ref)});
    }

    if (order.ownerName.contains(text,Qt::CaseInsensitive))
    {
        result.hits.push_back({SearchTarget::Order,order.id,toColumnIndex(WorkSheetOrderColumn::Owner)});
    }

    if (order.requiredColor.code().contains(text,Qt::CaseInsensitive))
    {
        result.hits.push_back({SearchTarget::Order,order.id,toColumnIndex(WorkSheetOrderColumn::Color)});
    }

    if (typeName.contains(text,Qt::CaseInsensitive))
    {
        result.hits.push_back({SearchTarget::Order,order.id,toColumnIndex(WorkSheetOrderColumn::TypeSubtype)});
    }

    if (subtypeName.contains(text,Qt::CaseInsensitive))
    {
        result.hits.push_back({SearchTarget::Order,order.id,toColumnIndex(WorkSheetOrderColumn::TypeSubtype)});
    }
}

SearchResult WorkSheetSummarySearchEngine::search(
    const WorkSheetSummary& summary,
    const QString& text)
{
    SearchResult result;

    QString txt = text.trimmed();

    if (txt.isEmpty())
    {
        return result;
    }

    if (summary.fileName.contains(txt,Qt::CaseInsensitive))
    {
        result.hits.push_back({SearchTarget::Summary,summary.id,0});
    }

    for (const auto& order : summary.orders)
    {
        searchOrder(order,txt,result);
    }

    return result;
}

SearchResult WorkSheetSummarySearchEngine::search(
    const QVector<WorkSheetSummary>& summaries,
    const QString& text)
{
    SearchResult result;

    for(const auto& summary : summaries)
    {
        auto partial =search(summary,text);
        result.hits += partial.hits;
    }

    return result;
}