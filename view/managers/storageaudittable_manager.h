#pragma once

#include <QObject>
#include <QTableWidget>
#include "common/tableutils/RowTracker.h"
//#include "common/tableutils/rowid.h"
#include "model/storageaudit/storageauditentry.h" // vagy ahová az entry kerül
#include "model/storageaudit/storageauditrow.h"

class StorageAuditTableManager : public QObject {
    Q_OBJECT

private:
    QTableWidget* _table;
    QWidget* _parent;
    RowTracker _rows;

public:
    explicit StorageAuditTableManager(QTableWidget* table, QWidget* parent = nullptr);

    void addRow(const StorageAuditRow& entry);
    void updateRow(const StorageAuditRow& row);    
    void clearTable();

private:
    void setStatusCell(QTableWidgetItem *item, const QString &status);

signals:
    void auditValueChanged(const QUuid& requestId, int actualQuantity);

public:
    static constexpr auto RowId_Key = "entryId"; // button eventekhez

    static constexpr int ColMaterial   = 0;
    static constexpr int ColStorage    = 1;
    static constexpr int ColExpected   = 2;
    static constexpr int ColActual     = 3;
    static constexpr int ColMissing    = 4;
    static constexpr int ColStatus     = 5;

};

