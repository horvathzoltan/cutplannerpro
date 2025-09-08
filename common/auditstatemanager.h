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

    void setTrackingEnabled(bool enabled) { _trackingEnabled = enabled; }
    bool isTrackingEnabled() const { return _trackingEnabled; }
signals:
    void auditStateChanged(bool outdated);

private:
    QSet<QUuid> auditedMaterialIds;
    QSet<QUuid> auditedStorageIds;
    QSet<QUuid> auditedStockIds;
    QSet<QUuid> auditedLeftoverIds;

    bool auditOutdated = false;    
    bool _trackingEnabled = true;

    void checkIfOutdated(const StockEntry& entry);
};
