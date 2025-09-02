#pragma once

#include <QObject>
#include <QTableWidget>
#include "model/cutting/plan/cutplan.h"

class ResultsTableManager : public QObject {
    Q_OBJECT

public:
    explicit ResultsTableManager(QTableWidget* table, QWidget* parent = nullptr);

    void addRow(const QString& rodNumber, const Cutting::Plan::CutPlan& plan);
    void clearTable();

private:
    QTableWidget* table;
    QWidget* parent;

    static constexpr int ColRod    = 0;
    static constexpr int ColGroup  = 1;
    static constexpr int ColLength = 2;
    static constexpr int ColKerf   = 3;
    static constexpr int ColWaste  = 4;
};
