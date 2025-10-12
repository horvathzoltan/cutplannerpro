#include "common/auditcontextbuilder.h"
#include "common/logger.h" // zInfo, zWarning
#include <QMap>

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
 * - AuditGroupInfo: materialId, groupKey, rowIds
 * - totalExpected: az adott anyagcsoport teljes elvárt mennyisége (aggregált, nem soronkénti!)
 * - totalActual: az adott anyagcsoport tényleges mennyisége (összegzett)
 * - Minden sorhoz visszaadjuk a saját context pointerét
 */
QHash<QUuid, std::shared_ptr<AuditContext>>
AuditContextBuilder::buildFromRows(const QList<StorageAuditRow>& rows,
                                   const QMap<QUuid,int>& requiredStockMaterials)
{
    // 1️⃣ Csoportosítás kulcsra: materialId + rootStorageId
    //    → minden anyag+hely kombináció egy csoportot alkot
    QHash<QString, QList<const StorageAuditRow*>> groups;
    groups.reserve(rows.size());

    for (const auto& r : rows) {
        groups[makeGroupKey(r)].push_back(&r);

        if(_isVerbose){
        // Debug log: minden sor bekerül a csoportba
        zInfo(L("[AuditContextBuilder] Row collected | rowId=%1 | matId=%2 | storage=%3 | rootStorage=%4 | picking=%5 | actual=%6")
                  .arg(r.rowId.toString())
                  .arg(r.materialId.toString())
                  .arg(r.storageName)
                  .arg(r.rootStorageId.toString())
                  .arg(r.pickingQuantity)
                  .arg(r.actualQuantity));
        }
    }

    // 2️⃣ Kontextus létrehozás és visszacsatolás rowId-hoz
    QHash<QUuid, std::shared_ptr<AuditContext>> result;
    result.reserve(rows.size());

    for (auto it = groups.begin(); it != groups.end(); ++it) {
        const QString& key = it.key();
        const auto& grp = it.value();
        if (grp.isEmpty()) continue;

        auto ctx = std::make_shared<AuditContext>(key, grp.first()->materialId);

        ctx->totalExpected = 0;
        ctx->totalActual   = 0;
        if(_isVerbose){
        zInfo(L("[AuditContextBuilder] Building context for groupKey=%1 | rows=%2")
                  .arg(key)
                  .arg(grp.size()));
        }
        // 2/A: tényleges mennyiség összegzése és sorok hozzárendelése
        for (const auto* r : grp) {
            ctx->totalActual += r->actualQuantity;
            ctx->group.addRow(r->rowId);

            if(_isVerbose){
            zInfo(L("   adding rowId=%1 | picking=%2 | actual=%3 | runningActual=%4")
                      .arg(r->rowId.toString())
                      .arg(r->pickingQuantity)   // sor szintű jelző, de nem aggregáljuk
                      .arg(r->actualQuantity)
                      .arg(ctx->totalActual));
            }
        }

        // 2/B: elvárt mennyiség beállítása anyagcsoport szinten a planből
        ctx->totalExpected = requiredStockMaterials.value(grp.first()->materialId, 0);

        if(_isVerbose){
        zInfo(L("   >> Context ready: expected=%1 | actual=%2 | rows=%3")
                  .arg(ctx->totalExpected)
                  .arg(ctx->totalActual)
                  .arg(ctx->group.size()));
        }

        // 3️⃣ Minden sorhoz hozzárendeljük a közös context pointert
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

