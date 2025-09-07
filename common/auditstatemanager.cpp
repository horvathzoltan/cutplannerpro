#include "auditstatemanager.h"

AuditStateManager::AuditStateManager(QObject* parent)
    : QObject(parent) {}

void AuditStateManager::setActiveAuditRows(const QVector<StorageAuditRow>& rows) {
    auditedMaterialIds.clear();
    auditedStorageIds.clear();
    auditOutdated = false;

    for (const auto& row : rows) {
        auditedMaterialIds.insert(row.materialId);
        auditedStorageIds.insert(row.storageId()); // vagy row.storageId, ha van
    }

    emit auditStateChanged(false);
}

void AuditStateManager::notifyStockChanged(const StockEntry& entry) {
    checkIfOutdated(entry);
}

void AuditStateManager::notifyStockAdded(const StockEntry& entry) {
    checkIfOutdated(entry);
}

void AuditStateManager::notifyStockRemoved(const QUuid& entryId) {
    // opcionálisan: keresd meg a StockRegistry-ből, és hívd meg checkIfOutdated()
}

void AuditStateManager::checkIfOutdated(const StockEntry& entry) {
    if (auditedMaterialIds.contains(entry.materialId) ||
        auditedStorageIds.contains(entry.storageId)) {
        if (!auditOutdated) {
            auditOutdated = true;
            emit auditStateChanged(true);
        }
    }
}

bool AuditStateManager::isAuditOutdated() const {
    return auditOutdated;
}
