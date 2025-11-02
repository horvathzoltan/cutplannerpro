#pragma once

#include <QObject>
#include <QTableWidget>
#include "model/cutting/plan/cutplan.h"

class ResultsTableManager : public QObject {
    Q_OBJECT

public:
    explicit ResultsTableManager(QTableWidget* table, QWidget* parent = nullptr);

    void addRow(const QString& rodNumber, const Cutting::Plan::CutPlan& plan);
    void addRow_old(const QString& rodNumber, const Cutting::Plan::CutPlan& plan);
    void clearTable();

private:
    QTableWidget* _table;
    QWidget* parent;

    static bool _isVerbose; ///< Debug log flag
};
