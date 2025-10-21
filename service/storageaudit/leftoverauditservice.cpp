#include "leftoverauditservice.h"
#include "common/logger.h"
#include "model/registries/leftoverstockregistry.h"

#include <QSet>

LeftoverAuditService::LeftoverAuditService(QObject* parent)
    : QObject(parent) {}

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
        row.pickingQuantity  = 0;            // globális auditnál nincs elvárt
        row.actualQuantity   = 1;            // regisztrált -> implicit 1 darab
        row.originalQuantity = 1;
        //row.rowPresence         = AuditPresence::Present; // implicit jelen
        row.isInOptimization = false;

        row.barcode = entry.reusableBarcode();         // Hulló azonosító
        row.storageName = entry.storageName();         // Tároló neve

        //row.isPresent      = true;           // ha ezt használjátok a UI-ban
        row.barcode        = entry.reusableBarcode();
        row.storageName    = entry.storageName();

        result.append(row);
    }

    zInfo(L("✅ Hulló audit sorok generálva: %1").arg(result.size()));
    return result;
}

