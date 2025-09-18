// common/auditcontextbuilder.cpp
#include "common/auditcontextbuilder.h"

QString AuditContextBuilder::makeGroupKey(const StorageAuditRow& r) {
    // Ha van külön auditGroupKey meződ, használd azt. Most: materialId + storageName
    return QString("%1|%2").arg(r.materialId.toString(), r.storageName);
}

QHash<QUuid, std::shared_ptr<AuditContext>>
AuditContextBuilder::buildFromRows(const QList<StorageAuditRow>& rows) {
    // 1) Csoportosítás kulcsra
    QHash<QString, QList<const StorageAuditRow*>> groups;
    groups.reserve(rows.size());

    for (const auto& r : rows) {
        groups[makeGroupKey(r)].push_back(&r);
    }

    // 2) Kontextus létrehozás és visszacsatolás rowId-hoz
    QHash<QUuid, std::shared_ptr<AuditContext>> result;
    result.reserve(rows.size());

    for (auto it = groups.begin(); it != groups.end(); ++it) {
        const QString& key = it.key();
        const auto& grp = it.value();
        if (grp.isEmpty()) continue;

        auto ctx = std::make_shared<AuditContext>();
        ctx->groupKey = key;
        ctx->materialId = grp.first()->materialId;
        ctx->totalExpected = 0;
        ctx->totalActual = 0;

        for (const auto* r : grp) {
            ctx->totalExpected += r->pickingQuantity;
            ctx->totalActual   += r->actualQuantity;
            ctx->rowIds.push_back(r->rowId);
        }

        for (const auto* r : grp) {
            result.insert(r->rowId, ctx);
        }
    }

    return result;
}
