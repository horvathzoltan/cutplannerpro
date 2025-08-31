#pragma once

#include <QObject>
#include <QTableWidget>
#include "common/tableutils/rowid.h"
#include "model/storageaudit/storageauditentry.h" // vagy ahová az entry kerül
#include "model/storageaudit/storageauditrow.h"

class StorageAuditTableManager : public QObject {
    Q_OBJECT

public:
    explicit StorageAuditTableManager(QTableWidget* table, QWidget* parent = nullptr);

    void addRow(const StorageAuditRow& entry);

    //void addRow_old(const StorageAuditEntry& entry);
    void clearTable();

private:
    QTableWidget* table;
    QWidget* parent;
    RowId _rowId;

    static constexpr int ColMaterial   = 0;
    static constexpr int ColStorage    = 1;
    static constexpr int ColExpected   = 2;
    static constexpr int ColActual     = 3;
    static constexpr int ColMissing    = 4;
    static constexpr int ColStatus = 5;
};

