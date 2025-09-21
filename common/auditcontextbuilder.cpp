#include "common/auditcontextbuilder.h"
#include "common/logger.h" // zInfo, zWarning

/**
 * @brief Csoportkulcs generálása egy audit sorhoz.
 *        A kulcs most: materialId + rootStorageId
 *        Ez biztosítja, hogy az audit sorok gépszinten csoportosuljanak.
 *        A hullók mind külön csoportot kapnak, így nem öröklik a hiányt, nem aggregálódnak.
 */
// QString AuditContextBuilder::makeGroupKey(const StorageAuditRow& r) {
//     if (r.sourceType == AuditSourceType::Leftover)
//         return r.rowId.toString(); // hullók egyediek, ne csoportosuljanak

//     return QString("%1|%2").arg(r.materialId.toString(), r.rootStorageId.toString());
// }
QString AuditContextBuilder::makeGroupKey(const StorageAuditRow& r) {
    if (r.sourceType == AuditSourceType::Leftover)
        return r.rowId.toString(); // hullók ne csoportosuljanak

    return QString("%1|%2").arg(r.materialId.toString(), r.rootStorageId.toString());
}


/**
 * @brief Audit sorok csoportosítása anyag + rootStorage szerint.
 *        Minden sorhoz létrejön egy AuditContext, amely tartalmazza a csoportosított adatokat.
 *
 * A context tartalmazza:
 * - AuditGroupInfo: materialId, groupKey, rowIds, totalExpected, totalActual
 * - Minden sorhoz visszaadjuk a saját context pointerét
 */
QHash<QUuid, std::shared_ptr<AuditContext>>
AuditContextBuilder::buildFromRows(const QList<StorageAuditRow>& rows)
{
    // 1️⃣ Csoportosítás kulcsra: materialId + rootStorageId
    QHash<QString, QList<const StorageAuditRow*>> groups;
    groups.reserve(rows.size());

    for (const auto& r : rows) {
        groups[makeGroupKey(r)].push_back(&r);

        // 🔍 Sor logolása: rowId, materialId, storageName, rootStorageId, expected, actual
        zInfo(L("[AuditContextBuilder] Row collected | rowId=%1 | matId=%2 | storage=%3 | rootStorage=%4 | expected=%5 | actual=%6")
                  .arg(r.rowId.toString())
                  .arg(r.materialId.toString())
                  .arg(r.storageName)
                  .arg(r.rootStorageId.toString())
                  .arg(r.pickingQuantity)
                  .arg(r.actualQuantity));
    }

    // 2️⃣ Kontextus létrehozás és visszacsatolás rowId-hoz
    QHash<QUuid, std::shared_ptr<AuditContext>> result;
    result.reserve(rows.size());

    for (auto it = groups.begin(); it != groups.end(); ++it) {
        const QString& key = it.key();
        const auto& grp = it.value();
        if (grp.isEmpty()) continue;

        // 🧩 Új AuditContext létrehozása a csoporthoz
        auto ctx = std::make_shared<AuditContext>();
        ctx->group.groupKey = key;
        ctx->group.materialId = grp.first()->materialId;
        ctx->group.totalExpected = 0;
        ctx->group.totalActual = 0;

        // 🔍 Csoport kezdete
        zInfo(L("[AuditContextBuilder] Building context for groupKey=%1 | rows=%2")
                  .arg(key)
                  .arg(grp.size()));

        for (const auto* r : grp) {
            ctx->group.totalExpected += r->pickingQuantity;
            ctx->group.totalActual   += r->actualQuantity;
            ctx->group.rowIds.push_back(r->rowId);

            // 🔍 Sor hozzáadása a contexthez
            zInfo(L("   adding rowId=%1 | expected=%2 | actual=%3 | totals → expected=%4, actual=%5")
                      .arg(r->rowId.toString())
                      .arg(r->pickingQuantity)
                      .arg(r->actualQuantity)
                      .arg(ctx->group.totalExpected)
                      .arg(ctx->group.totalActual));
        }

        // 3️⃣ Minden sorhoz hozzárendeljük a context pointert
        for (const auto* r : grp) {
            result.insert(r->rowId, ctx);
        }
    }

    // ✅ Összegzés
    zInfo(L("📊 AuditContextBuilder kész: %1 csoport, %2 sor")
              .arg(groups.size())
              .arg(rows.size()));

    return result;
}
