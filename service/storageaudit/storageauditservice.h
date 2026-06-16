#pragma once

#include <QObject>
#include <QVector>
#include "../../model/cutting/cuttingmachine.h"
#include "../../model/stockentry.h"
//#include "model/storageaudit/storageauditentry.h"
#include "../../model/storageaudit/storageauditrow.h"

class StorageAuditService : public QObject {
    Q_OBJECT

public:

    struct MachineStorageAudit {
        bool hasStorage = false;          // van-e storage hozzárendelve
        bool hasStockInStorage = false;   // van-e bármilyen készlet a storage-ben
        QVector<StorageAuditRow> rows;    // a tényleges készlet
    };

    explicit StorageAuditService(QObject* parent = nullptr);

    static StorageAuditRow createAuditRow(const StockEntry& stock, const QUuid& rootStorageId);
    static QVector<StorageAuditRow> generateAuditRows_All();
    //static AuditMode _mode;
    static MachineStorageAudit auditMachineStorage(const CuttingMachine& machine);
};
