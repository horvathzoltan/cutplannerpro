#include "leftoverauditservice.h"
#include "common/logger.h"
#include "model/registries/leftoverstockregistry.h"

#include <QSet>

LeftoverAuditService::LeftoverAuditService(QObject* parent)
    : QObject(parent) {}

/**
 * @brief Teljes leftover audit sorok legenerálása.
 *
 * Lépései:
 * - Lekéri az összes leftover bejegyzést a LeftoverStockRegistry-ből.
 * - Minden leftover bejegyzésből StorageAuditRow készül, forrás típusa: Leftover.
 * - Minden leftover sor egyedi példányt képvisel (külön barcode, külön entryId).
 * - A sor implicit actualQuantity=1 értéket kap, mert a leftover mindig egy darab.
 *
 * Eredmény:
 * - Egy vektor, amely tartalmazza az összes leftover audit sort.
 *
 * Fontos:
 * - Ezek a sorok nem aggregálódnak materialId szerint.
 * - A context építésnél mindig külön contextbe kerülnek (rowId alapján).
 * - Az expected értékük bináris (0/1), amit az injectPlansIntoAuditRows állít be
 *   attól függően, hogy a plan ténylegesen felhasználja-e az adott leftover példányt.
 *
 * @return QVector<StorageAuditRow> Az összes leftover audit sor.
 */

QVector<StorageAuditRow> LeftoverAuditService::generateAuditRows_All()
{
    QVector<StorageAuditRow> result;

    const auto entries = LeftoverStockRegistry::instance().readAll();
    zInfo(L("♻️ Hulló audit indítása — regisztrált hullók száma: %1").arg(entries.size()));

    for (const auto& entry : entries) {
        StorageAuditRow row;
        row.rowId = QUuid::createUuid();               // Egyedi azonosító az audit sorhoz
        row.materialId = entry.materialId;             // Anyag azonosító
        row.stockEntryId = entry.entryId;              // Kapcsolat a hullóhoz
        row.sourceType = AuditSourceType::Leftover;    // Forrás: hulló

        // Példányszintű modell: 1 bejegyzés == 1 darab
        //row.pickingQuantity  = 0;            // globális auditnál nincs elvárt
        row.actualQuantity   = 1;            // regisztrált -> implicit 1 darab
        row.originalQuantity = 1;
        //row.rowPresence         = AuditPresence::Present; // implicit jelen
        row.isInOptimization = false;

        row.barcode = entry.barcode;         // Hulló azonosító
        row.storageName = entry.storageName();         // Tároló neve

        result.append(row);
    }

    zInfo(L("✅ Hulló audit sorok generálva: %1").arg(result.size()));
    return result;
}

