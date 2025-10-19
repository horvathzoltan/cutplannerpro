#include "auditstatemanager.h"

AuditStateManager::AuditStateManager(QObject* parent)
    : QObject(parent) {}

void AuditStateManager::setActiveAuditRows(const QVector<StorageAuditRow>& rows) {
    auditedMaterialIds.clear();
    auditedStorageIds.clear();
    auditedStockIds.clear();
    auditedLeftoverIds.clear();
    auditOutdated = false;

    for (const auto& row : rows) {
        //auditedStockIds.insert(row.stockEntryId);
        if (row.sourceType == AuditSourceType::Stock) {
            auditedStockIds.insert(row.stockEntryId);
        } else if (row.sourceType == AuditSourceType::Leftover) {
            auditedLeftoverIds.insert(row.stockEntryId);
        }

        auditedMaterialIds.insert(row.materialId);
        auditedStorageIds.insert(row.storageId());
    }

    auditOutdated = false;
    emit auditStateChanged(false);
}

void AuditStateManager::notifyStockChanged(const StockEntry& entry) {
    checkIfOutdated(entry);
}

void AuditStateManager::notifyStockAdded(const StockEntry& entry) {
    checkIfOutdated(entry);
}

void AuditStateManager::notifyStockRemoved(const QUuid& entryId) {
    if (auditedStockIds.contains(entryId)) {
        if (!auditOutdated) {
            auditOutdated = true;
            emit auditStateChanged(true);
        }
    }
}

void AuditStateManager::checkIfOutdated(const StockEntry& entry) {
    if (!_trackingEnabled)
        return;
    if (auditedMaterialIds.contains(entry.materialId) ||
        auditedStorageIds.contains(entry.storageId) ||
        auditedStockIds.contains(entry.entryId)) {
        if (!auditOutdated) {
            auditOutdated = true;
            emit auditStateChanged(true);
        }
    }
}

bool AuditStateManager::isAuditOutdated() const {
    return auditOutdated;
}
