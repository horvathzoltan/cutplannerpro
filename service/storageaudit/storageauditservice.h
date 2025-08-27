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
    enum class AuditMode {
        Passive,   // csak nézelődünk
        Expected,  // van picking list
    };

    explicit StorageAuditService(QObject* parent = nullptr);

    static QVector<StorageAuditRow> generateAuditRows(const QMap<QString, int>& pickingMap);
    static StorageAuditRow createAuditRow(const StockEntry& stock, const QString& storageName, const QMap<QString, int>& pickingMap);
private:
    static AuditMode _mode;
    static QVector<StorageAuditRow> auditMachineStorage(const CuttingMachine& machine, const QMap<QString, int>& pickingMap);
};
