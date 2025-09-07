#pragma once

#include <QObject>
#include <QUuid>
#include <QSet>
#include <QMap>
#include "model/storageaudit/storageauditrow.h"
#include "model/stockentry.h"

class AuditStateManager : public QObject {
    Q_OBJECT
public:
    explicit AuditStateManager(QObject* parent = nullptr);

    void setActiveAuditRows(const QVector<StorageAuditRow>& rows);
    void notifyStockChanged(const StockEntry& entry);
    void notifyStockAdded(const StockEntry& entry);
    void notifyStockRemoved(const QUuid& entryId);

    bool isAuditOutdated() const;

signals:
    void auditStateChanged(bool outdated);

private:
    QSet<QUuid> auditedMaterialIds;
    QSet<QUuid> auditedStorageIds;
    bool auditOutdated = false;

    void checkIfOutdated(const StockEntry& entry);
};
