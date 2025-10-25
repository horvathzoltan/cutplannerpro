#pragma once

#include <QTableWidget>
#include <QWidget>
//#include <QUuid>

#include "view/tableutils/RowTracker.h"

#include "model/stockentry.h"
//#include "model/materialmaster.h"


class StockTableManager: public QObject {  // 🔧 QObject öröklés! {
    Q_OBJECT                              // ✨ Qt metaobjektum makró!

private:
    QTableWidget* _table;
    QWidget* _parent;
    RowTracker _rows;

public:
    StockTableManager(QTableWidget* table, QWidget* parent = nullptr);

    void addRow(const StockEntry& entry);
    void updateRow(const StockEntry &entry);
    void removeRowById(const QUuid &stockId);
    void refresh_TableFromRegistry();

signals:
    void deleteRequested(const QUuid& requestId);
    void editRequested(const QUuid& requestId);
    void editQtyRequested(const QUuid& requestId);
    void editStorageRequested(const QUuid& requestId);
    void editCommentRequested(const QUuid& requestId);
    void moveRequested(const QUuid& requestId);

public:
    static constexpr auto EntryId_Key = "entryId"; // button eventekhez

    static constexpr int ColName     = 0;
    static constexpr int ColBarcode  = 1;
    static constexpr int ColLength   = 2;
    static constexpr int ColShape    = 3;
    static constexpr int ColQuantity = 4;
    static constexpr int ColStorageName = 5;
    static constexpr int ColComment  = 6;
    static constexpr int ColAction = 7;  // oszlop a gomboknak - legutolsó


};


