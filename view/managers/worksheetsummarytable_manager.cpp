#include "worksheetsummarytable_manager.h"

#include "../tableutils/tableutils.h"

#include "worksheetsummary/registry/worksheetsummary_registry.h"

#include <QTableWidgetItem>

bool WorkSheetSummaryTableManager::_isVerbose = false;

WorkSheetSummaryTableManager::WorkSheetSummaryTableManager(
    QTableWidget* table, QWidget* parent)
    : QObject(parent), _table(table), parent(parent)
{
}

void WorkSheetSummaryTableManager::addRow(
    const WorkSheetSummary& summary)
{
    if (!_table)
        return;

    int rowIx =
        _table->rowCount();

    _table->insertRow(rowIx);

    _table->setItem(
        rowIx,
        0,
        new QTableWidgetItem(
            summary.fileName));

    _table->setItem(
        rowIx,
        1,
        new QTableWidgetItem(
            WorkSheetPresenceUtils::toDisplayText(summary.presence)));

    _table->setItem(
        rowIx,
        2,
        new QTableWidgetItem(
            QString::number(
                summary.requestCount())));

    _table->setItem(
        rowIx,
        3,
        new QTableWidgetItem(
            QString::number(
                summary.completedCount())));

    _table->setItem(
        rowIx,
        4,
        new QTableWidgetItem(
            QString::number(
                summary.uncompletedCount())));

    _table->setItem(
        rowIx,
        5,
        new QTableWidgetItem(
            QString::number(
                summary.undefinedCount())));
    _table->setItem(
        rowIx,
        6,
        new QTableWidgetItem(
            summary.requiresAttention()? "Yes" : "No")
        );

    if (_isVerbose)
    {
        qDebug()
            << "Worksheet row added:"
            << summary.fileName;
    }
}

void WorkSheetSummaryTableManager::
    refresh_TableFromRegistry()
{
    if (!_table)
        return;

    TableUtils::clearSafely(
        _table);

    const auto& data =
        WorkSheetSummaryRegistry::instance()
            .readAll();

    for (const auto& item : data)
    {
        addRow(item);
    }
}

void WorkSheetSummaryTableManager::clearTable()
{
    if (!_table)
        return;

    _table->setRowCount(0);
    _table->clearContents();
}