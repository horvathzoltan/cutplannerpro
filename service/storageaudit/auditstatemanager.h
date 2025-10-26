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

    enum class AuditOutdatedReason {
        None,
        OptimizeRun,
        StockChanged,
        LeftoverChanged,
        RelocationFinalized
        };

    void setOutdated(AuditOutdatedReason reason) {
        if (!_trackingEnabled) return;
        if (!auditOutdated || lastReason != reason) {
                auditOutdated = (reason != AuditOutdatedReason::None);
                lastReason = reason;
                emit auditStateChanged(reason);
            }
    }

    void setActiveAuditRows(const QVector<StorageAuditRow>& rows);
    void notifyStockChanged(const StockEntry& entry);
    void notifyStockAdded(const StockEntry& entry);
    void notifyStockRemoved(const QUuid& entryId);

    bool isAuditOutdated() const { return auditOutdated; }
    AuditOutdatedReason reason() const { return lastReason; }

    void setTrackingEnabled(bool enabled) { _trackingEnabled = enabled; }
    bool isTrackingEnabled() const { return _trackingEnabled; }
signals:
    void auditStateChanged(AuditOutdatedReason reason);

private:
    QSet<QUuid> auditedMaterialIds;
    QSet<QUuid> auditedStorageIds;
    QSet<QUuid> auditedStockIds;
    QSet<QUuid> auditedLeftoverIds;

    bool auditOutdated = false;    
    bool _trackingEnabled = true;

    AuditOutdatedReason lastReason = AuditOutdatedReason::None;
    void checkIfOutdated(const StockEntry& entry);
};
