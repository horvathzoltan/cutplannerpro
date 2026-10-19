#pragma once

#include <QObject>
#include <QTableWidget>

#include "worksheetsummary/model/worksheetsummary.h"

class WorkSheetSummaryTableManager : public QObject
{
    Q_OBJECT

public:
    explicit WorkSheetSummaryTableManager(QTableWidget* table, QWidget* parent = nullptr);

    void addRow(
        const WorkSheetSummary& summary);

    void refresh_TableFromRegistry();

    void clearTable();

private:

    QTableWidget* _table;
    QWidget* parent;

    static bool _isVerbose;
};