#pragma once

#include "worksheetsummary/model/worksheetsummary.h"

#include <QWidget>
#include <QHash>

#include "worksheetorderrowwidget.h"
#include "search/model/searchresult.h"

class WorkSheetSummaryDetailWidget
    : public QWidget
{
    Q_OBJECT

public:

    explicit WorkSheetSummaryDetailWidget(
        const WorkSheetSummary& summary,
        QWidget* parent = nullptr);

    void applySearchResult(
        const SearchResult& result);

    void clearSearchHighlights();

private:

    QHash<QUuid, OrderRowUi> _orderRows;
};
