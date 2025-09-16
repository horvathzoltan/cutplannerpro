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

        row.pickingQuantity = 0;//pickingMap.value(stock.materialBarcode(), 0); // vagy materialName alapján
        row.actualQuantity = 0;                        // Audit során még nincs megerősítve
        row.presence = AuditPresence::Unknown;         // Felhasználó fogja beállítani
        row.isInOptimization = false;                  // Később validálható CutPlan alapján

       // row.isPresent        = true;//entry.quantity > 0;    // Ha van mennyiség, akkor jelen van

        row.barcode = entry.reusableBarcode();         // Hulló azonosító
        row.storageName = entry.storageName();         // Tároló neve

        result.append(row);
    }

    zInfo(L("✅ Hulló audit sorok generálva: %1").arg(result.size()));
    return result;
}

