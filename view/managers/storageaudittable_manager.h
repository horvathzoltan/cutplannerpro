#pragma once

#include <QObject>
#include <QTableWidget>
#include <common/tableutils/auditgrouplabeler.h>
#include "common/tableutils/RowTracker.h"
//#include "common/tableutils/rowid.h"
#include "model/storageaudit/storageauditentry.h" // vagy ahová az entry kerül
#include "model/storageaudit/storageauditrow.h"
#include "common/tableutils/auditgroupsynchronizer.h"


class StorageAuditTableManager : public QObject {
    Q_OBJECT

private:
    QTableWidget* _table;
    QWidget* _parent;
    RowTracker _rows;
    QMap<QUuid, StorageAuditRow> _auditRowMap;
    std::unique_ptr<TableUtils::AuditGroupSynchronizer> _groupSync;
    TableUtils::AuditGroupLabeler _groupLabeler;


public:
    explicit StorageAuditTableManager(QTableWidget* table, QWidget* parent = nullptr);

    void addRow(const StorageAuditRow& entry);
    void updateRow(const StorageAuditRow& row);
    void clearTable();

    //void createAuditRowWidgets(const StorageAuditRow &row, int rowIx);
    //void populateAuditRowContent(const StorageAuditRow &row, int rowIx, const QString &groupLabel);

private:
    //void setStatusCell(QTableWidgetItem *item, const QString &status);

    //void applyGroupContextToRows(const StorageAuditRow &row);    
signals:
    void auditValueChanged(const QUuid& requestId, int actualQuantity);
    void leftoverPresenceChanged(const QUuid& rowId, AuditPresence presence);
public:
    static constexpr auto RowId_Key = "entryId"; // button eventekhez

    // static constexpr int ColMaterial   = 0;
    // static constexpr int ColBarcode    = 1;
    // static constexpr int ColStorage    = 2;
    // static constexpr int ColExpected   = 3;
    // static constexpr int ColActual     = 4;
    // static constexpr int ColMissing    = 5;
    // static constexpr int ColStatus     = 6;

};

