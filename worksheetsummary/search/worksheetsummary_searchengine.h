#pragma once

#include "search/model/searchresult.h"
#include "worksheetsummary/model/worksheetsummary.h"

class WorkSheetSummarySearchEngine
{
public:

    static SearchResult search(
        const QVector<WorkSheetSummary>& summaries,
        const QString& text);

    static SearchResult search(
        const WorkSheetSummary& summary,
        const QString& text);

private:

    static void searchOrder(
        const WorkSheetOrderInfo& order,
        const QString& text,
        SearchResult& result);
};