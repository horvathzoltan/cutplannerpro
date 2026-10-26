#pragma once

#include <QObject>
#include <QTableWidget>

#include <search/model/searchresult.h>

#include "worksheetsummary/model/worksheetsummary.h"
#include "view/tableutils/rowtracker2.h"
#include <QHash>

class WorkSheetSummaryDetailWidget;

class WorkSheetSummaryTableManager : public QObject
{
    Q_OBJECT

public:
    explicit WorkSheetSummaryTableManager(QTableWidget* table, QWidget* parent = nullptr);

    void addRow(
        const WorkSheetSummary& summary);

    void refresh_TableFromRegistry();

    void clearTable();

    enum class RowType
    {
        Summary,
        Detail
    };


    void applySearchResult(
        const SearchResult& result);

    void clearSearchHighlights();

private:

    QTableWidget* _table;
    QWidget* parent;

    RowTracker2 _rows;

    QHash<QUuid, WorkSheetSummaryDetailWidget*> _detailWidgets;

    static bool _isVerbose;
};