#pragma once

#include <QObject>
#include <QVector>
#include "model/cutting/cuttingmachine.h"
#include "model/stockentry.h"
#include "model/storageaudit/storageauditentry.h"




class StorageAuditService : public QObject {
    Q_OBJECT

public:
    enum class AuditMode {
        Passive,   // csak nézelődünk
        Expected,  // van picking list
    };

    explicit StorageAuditService(QObject* parent = nullptr);

    static QVector<StorageAuditEntry> generateAuditEntries();

    static StorageAuditEntry createAuditEntry(const StockEntry &stock, const QString &storageName);
private:
    static AuditMode _mode;
    static QVector<StorageAuditEntry> auditMachineStorage(const CuttingMachine& machine);
};
