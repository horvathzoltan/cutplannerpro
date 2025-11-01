#include "auditcontextbuilder.h"
#include "common/logger.h" // zInfo, zWarning
#include <QMap>

/**
 * @brief Egyedi csoportkulcs generálása egy audit sorhoz.
 *
 * - Stock sorok esetén: a kulcs = materialId + rootStorageId.
 *   Ez biztosítja, hogy az azonos anyagok ugyanazon géphez tartozó készletei
 *   közös contextbe kerüljenek, és aggregáltan számolódjon az elvárt mennyiség.
 *
 * - Leftover sorok esetén: a kulcs = rowId.
 *   Így minden leftover példány külön contextet kap, nem aggregálódik materialId szerint,
 *   és nem keveredik a stock sorokkal.
 *
 * @param r Az audit sor, amelyhez a kulcsot képezzük.
 * @return QString A csoportosítási kulcs.
 */
QString AuditContextBuilder::makeGroupKey(const StorageAuditRow& r) {
    if (r.sourceType == AuditSourceType::Leftover)
        return r.rowId.toString(); // hullók ne csoportosuljanak

    return QString("%1|%2").arg(r.materialId.toString(), r.rootStorageId.toString());
}

/**
 * @brief AuditContext objektumok építése az audit sorokból.
 *
 * Feladata:
 * - A sorokat csoportosítja a makeGroupKey() alapján.
 *   - Stock sorok: materialId + rootStorageId szerint.
 *   - Leftover sorok: mindig saját rowId szerint, így önálló contextet kapnak.
 *
 * - Minden csoporthoz létrehoz egy AuditContext-et, amely tartalmazza:
 *   - totalExpected:
 *       * Stock esetén → a pickingMap (requiredStockMaterials) alapján számolt darabszám.
 *       * Leftover esetén → bináris érték (0/1), attól függően, hogy a sor isInOptimization=true-e.
 *   - totalActual: a tényleges készletmennyiség (stocknál raktári darabszám, leftovernél mindig 1).
 *   - group: a csoporthoz tartozó sorok azonosítói.
 *
 * - A visszatérési érték egy map (rowId → AuditContext pointer), amely minden sorhoz
 *   hozzárendeli a megfelelő contextet.
 *
 * Ez a függvény biztosítja, hogy a stock sorok aggregáltan, a leftover sorok pedig
 * példány szinten, önállóan jelenjenek meg az auditban.
 *
 * @param rows Az audit sorok listája.
 * @param requiredStockMaterials A pickingMap, amely a stock anyagok elvárt darabszámát tartalmazza.
 * @return QHash<QUuid, std::shared_ptr<AuditContext>> rowId → AuditContext hozzárendelés.
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
                  .arg(r.totalExpected())
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
                      .arg(r->totalExpected()) // sor szintű jelző, de nem aggregáljuk
                      .arg(r->actualQuantity)
                      .arg(ctx->totalActual));
            }
        }

        // 2/B: elvárt mennyiség beállítása anyagcsoport szinten a planből
        //ctx->totalExpected = requiredStockMaterials.value(grp.first()->materialId, 0);

        // ctx->totalExpected = 0;
        // for (const auto* r : grp) {
        //     ctx->totalExpected = std::max(ctx->totalExpected,
        //                                   requiredStockMaterials.value(r->materialId, 0));
        // }

        // 2/B: elvárt mennyiség beállítása anyagcsoport szinten a planből
        // 2/B: elvárt mennyiség beállítása forrástípus szerint
        if (grp.first()->sourceType == AuditSourceType::Stock) {
            ctx->totalExpected = requiredStockMaterials.value(grp.first()->materialId, 0);
        } else if (grp.first()->sourceType == AuditSourceType::Leftover) {
            ctx->totalExpected = 0;
            for (const auto* r : grp) {
                if (r->isInOptimization) {
                    ctx->totalExpected = 1;
                    break;
                }
            }
        } else {
            ctx->totalExpected = 0;
        }


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

