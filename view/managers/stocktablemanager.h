#pragma once

#include <QTableWidget>
#include <QWidget>
#include <QUuid>
//#include "model/materialmaster.h"
#include "model/stockentry.h"

class StockTableManager: public QObject {  // 🔧 QObject öröklés! {
    Q_OBJECT                              // ✨ Qt metaobjektum makró!

public:
    StockTableManager(QTableWidget* table, QWidget* parent = nullptr);

    void addRow(const StockEntry& entry);

    void refresh_TableFromRegistry();

signals:
    void deleteRequested(const QUuid& requestId);
    void editRequested(const QUuid& requestId);
    void editQtyRequested(const QUuid& requestId);
    void editStorageRequested(const QUuid& requestId);
    void editCommentRequested(const QUuid& requestId);
    void moveRequested(const QUuid& requestId);

private:
    QTableWidget* table;
    QWidget* parent;

    static constexpr int StockEntryIdIdRole = Qt::UserRole + 1;

public:
    static constexpr int ColName     = 0;
    static constexpr int ColBarcode  = 1;
    static constexpr int ColLength   = 2;
    static constexpr int ColShape    = 3;
    static constexpr int ColQuantity = 4;
    static constexpr int ColStorageName = 5;
    static constexpr int ColComment = 6;

    static constexpr int ColAction = 7;  // új oszlop a gomboknak (a Quantity után)


    void updateRow(const StockEntry &entry);
    void removeRowById(const QUuid &stockId);
};
