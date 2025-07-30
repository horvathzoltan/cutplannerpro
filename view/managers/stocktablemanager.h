#pragma once

#include <QTableWidget>
#include <QWidget>
#include <QUuid>
//#include "model/materialmaster.h"
#include "model/stockentry.h"

class StockTableManager {
public:
    StockTableManager(QTableWidget* table, QWidget* parent = nullptr);

    void addRow(const StockEntry& entry);

    void updateTableFromRegistry();

private:
    QTableWidget* table;
    QWidget* parent;

public:
    static constexpr int ColName     = 0;
    static constexpr int ColBarcode  = 1;
    static constexpr int ColShape    = 3;
    static constexpr int ColLength   = 2;
    static constexpr int ColQuantity = 4;
    void updateRow(int row, const StockEntry &entry);
};
