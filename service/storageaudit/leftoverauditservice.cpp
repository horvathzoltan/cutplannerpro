#include "leftoverauditservice.h"
#include "model/registries/leftoverstockregistry.h"

LeftoverAuditService::LeftoverAuditService(QObject* parent)
    : QObject(parent)
{}

LeftoverAuditService& LeftoverAuditService::instance() {
    static LeftoverAuditService service;
    return service;
}

QVector<StorageAuditRow> LeftoverAuditService::generateAudit() {
    const auto leftovers = LeftoverStockRegistry::instance().readAll();
    return generateAudit(leftovers); // delegálás
}

QVector<StorageAuditRow> LeftoverAuditService::generateAudit(const QVector<LeftoverStockEntry>& entries) {
    QVector<StorageAuditRow> result;

    for (const auto& entry : entries) {
        StorageAuditRow row;
        row.rowId = QUuid::createUuid();
        row.materialId = entry.materialId;
        row.stockEntryId = entry.entryId;
        row.sourceType = AuditSourceType::Leftover;
        row.actualQuantity = 0; // default: nincs ott, majd a felhasználó pipálja
        result.append(row);
    }

    return result;
}
