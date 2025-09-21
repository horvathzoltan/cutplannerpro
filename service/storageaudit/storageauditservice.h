#pragma once

#include <QObject>
#include <QVector>
#include "model/cutting/cuttingmachine.h"
#include "model/stockentry.h"
//#include "model/storageaudit/storageauditentry.h"
#include "model/storageaudit/storageauditrow.h"

class StorageAuditService : public QObject {
    Q_OBJECT

public:
    // enum class AuditMode {
    //     Passive,   // csak nézelődünk
    //     Expected,  // van picking list
    // };

    explicit StorageAuditService(QObject* parent = nullptr);

    static StorageAuditRow createAuditRow(const StockEntry& stock, const QUuid& rootStorageId);
    static QVector<StorageAuditRow> generateAuditRows_All();
private:
    //static AuditMode _mode;
    static QVector<StorageAuditRow> auditMachineStorage(const CuttingMachine& machine);
};
