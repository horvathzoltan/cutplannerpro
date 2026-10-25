#pragma once

#include <QObject>
#include <QTableWidget>

#include <search/model/searchresult.h>

#include "worksheetsummary/model/worksheetsummary.h"
#include "view/tableutils/rowtracker2.h"


class WorkSheetSummaryTableManager : public QObject
{
    Q_OBJECT

public:
    explicit WorkSheetSummaryTableManager(QTableWidget* table, QWidget* parent = nullptr);

    void addRow(
        const WorkSheetSummary& summary);

    void refresh_TableFromRegistry();

    void clearTable();
    void setSearchText(const QString& txt);

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
    QString _searchText;

    RowTracker2 _rows;

    static bool _isVerbose;
};