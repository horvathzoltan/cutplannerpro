#include "worksheetsummarytable_manager.h"
#include "../tableutils/tableutils.h"
#include "worksheetsummary/registry/worksheetsummary_registry.h"
#include <QTableWidgetItem>
#include <algorithm>
#include <product/registry/product_subtype_registry.h>
#include <product/registry/product_type_registry.h>
#include <view/common/widgets/worksheetsummarydetailwidget.h>

bool WorkSheetSummaryTableManager::_isVerbose = false;

WorkSheetSummaryTableManager::WorkSheetSummaryTableManager(
    QTableWidget* table, QWidget* parent)
    : QObject(parent), _table(table), parent(parent),_rows(table)
{
}

void WorkSheetSummaryTableManager::addRow(
    const WorkSheetSummary& summary)
{
    if (!_table)return;

    int rowIx =_table->rowCount();

    _table->insertRow(rowIx);

    _rows.registerRow(rowIx,
                      summary.id,static_cast<int>(RowType::Summary));

    _table->setItem(rowIx,0,
                    new QTableWidgetItem(summary.fileName));

    _table->setItem(rowIx,1,
        new QTableWidgetItem(WorkSheetPresenceUtils::toDisplayText(summary.presence)));

    _table->setItem(rowIx,2,
        new QTableWidgetItem(QString::number(summary.requestCount())));

    _table->setItem(rowIx,3,
        new QTableWidgetItem(QString::number(summary.completedCount())));

    _table->setItem(rowIx,4,
        new QTableWidgetItem(QString::number(summary.uncompletedCount())));

    _table->setItem(rowIx,5,
        new QTableWidgetItem(QString::number(summary.undefinedCount())));

    _table->setItem(rowIx,6,
        new QTableWidgetItem(summary.requiresAttention()? "Yes" : "No"));

    if (_isVerbose)
    {
        qDebug()
            << "Worksheet row added:"
            << summary.fileName;
    }

    if (!summary.orders.isEmpty())
    {
        int detailRowIx =
            _table->rowCount();

        _table->insertRow(detailRowIx);

        _rows.registerRow(detailRowIx,
                          summary.id,static_cast<int>(RowType::Detail));

        auto* widget =
            new WorkSheetSummaryDetailWidget(summary,_table);

        _detailWidgets.insert(summary.id,widget);

        widget->adjustSize();

        _table->setSpan(detailRowIx,0,1,_table->columnCount());

        _table->setCellWidget(detailRowIx,0,widget);

        _table->setRowHeight(detailRowIx,widget->sizeHint().height());

    }
}

void WorkSheetSummaryTableManager:: refresh_TableFromRegistry()
{
    if (!_table) return;

    _detailWidgets.clear();
    TableUtils::clearSafely(_table);

    const auto& data =WorkSheetSummaryRegistry::instance().readAll();

    for (const auto& item : data)
    {
        addRow(item);
    }
}

void WorkSheetSummaryTableManager::clearTable()
{
    if (!_table) return;

    _detailWidgets.clear();

    _table->setRowCount(0);
    _table->clearContents();
}

void WorkSheetSummaryTableManager::clearSearchHighlights()
{
    if (!_table) return;

    for (int row = 0;row < _table->rowCount();++row)
    {
        for (int col = 0;col < _table->columnCount();++col)
        {
            auto* item =
                _table->item(
                    row,
                    col);

            if (item)
            {
                item->setBackground(
                    QBrush());
            }
        }
    }

    for (auto it =
         _detailWidgets.begin();
         it != _detailWidgets.end();
         ++it)
    {
        if (it.value())
        {
            it.value()->clearSearchHighlights();
        }
    }
}

void WorkSheetSummaryTableManager::applySearchResult(const SearchResult& result)
{
    clearSearchHighlights();

    QColor hitColor(
        "#704040");

    for (const SearchHit& hit: result.hits)
    {
        if (hit.target != SearchTarget::Summary)
        {
            continue;
        }

        auto rowOpt =_rows.rowOf(hit.id);

        if (!rowOpt)
        {
            continue;
        }

        int row =*rowOpt;

        auto* item =_table->item(row,hit.column);

        if (!item)
        {
            continue;
        }

        item->setBackground(hitColor);
    }

    for (auto it =_detailWidgets.begin();it != _detailWidgets.end();++it)
    {
        if (it.value())
        {
            it.value()->applySearchResult(result);
        }
    }
}