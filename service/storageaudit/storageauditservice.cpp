#include "storageauditservice.h"
#include "model/registries/cuttingmachineregistry.h"
#include "model/registries/storageregistry.h"
#include "model/registries/stockregistry.h"
#include "common/logger.h"
#include "model/storageaudit/storageauditrow.h"

#include <QMultiMap>

//StorageAuditService::AuditMode StorageAuditService::_mode = AuditMode::Passive;

StorageAuditService::StorageAuditService(QObject* parent)
    : QObject(parent) {}

/**
 * @brief Teljes stock audit sorok legenerálása minden vágógéphez.
 *
 * Lépései:
 * - Lekéri az összes regisztrált vágógépet a CuttingMachineRegistry-ből.
 * - Minden géphez meghívja az auditMachineStorage() függvényt,
 *   amely kigyűjti a rootStorage és az alatta lévő tárolók stock bejegyzéseit.
 * - Minden stock bejegyzésből StorageAuditRow készül, forrás típusa: Stock.
 *
 * Eredmény:
 * - Egy vektor, amely tartalmazza az összes stock audit sort,
 *   gépenként és tárolónként kigyűjtve.
 *
 * Fontos:
 * - Ezek a sorok anyag szinten aggregálódnak majd a context építésnél.
 * - Az expected értékük a pickingMap alapján kerül kiszámításra,
 *   nem itt a generálásnál.
 *
 * @return QVector<StorageAuditRow> Az összes stock audit sor.
 */

QVector<StorageAuditRow> StorageAuditService::generateAuditRows_All()
{
    QVector<StorageAuditRow> result;

    const auto& machines = CuttingMachineRegistry::instance().readAll();
    zInfo(L("🔍 Teljes audit futtatása minden gépre — gépek száma: %1").arg(machines.size()));

    for (const auto& machine : machines) {
        zInfo(L("📦 Auditálás géphez: %1, rootStorageId: %2")
                  .arg(machine.name)
                  .arg(machine.rootStorageId.toString()));

        auto machineEntries = auditMachineStorage(machine); // minden anyag

        zInfo(L("📋 Audit bejegyzések száma géphez [%1]: %2")
                  .arg(machine.name)
                  .arg(machineEntries.size()));

        result += machineEntries;
    }

    zInfo(L("✅ Összes audit bejegyzés (stock): %1").arg(result.size()));
    return result;
}

QVector<StorageAuditRow> StorageAuditService::auditMachineStorage(const CuttingMachine& machine)
{
    QVector<StorageAuditRow> rows;

    const auto& storageEntries = StorageRegistry::instance().findByParentId(machine.rootStorageId);
    zInfo(L("Tárolók száma géphez [%1]: %2").arg(machine.name).arg(storageEntries.size()));

    const auto& stockEntries = StockRegistry::instance().readAll();
    zInfo(L("Összes készletbejegyzés: %1").arg(stockEntries.size()));


    for (const auto& storage : storageEntries) {
        zInfo(L("Tároló [%1] ID: %2").arg(storage.name).arg(storage.id.toString()));
    }

    for (const auto& stock : stockEntries) {
        zInfo(L("Készlet [%1] storageId: %2").arg(stock.materialName()).arg(stock.storageId.toString()));
    }

    QMultiMap<QUuid, StockEntry> stockByStorage;
    for (const auto& stock : StockRegistry::instance().readAll()) {
        stockByStorage.insert(stock.storageId, stock);
    }

    // tároló gyökérelem tartalmának a kigyűjtése
    const auto rootStocks = stockByStorage.values(machine.rootStorageId);
    for (const auto& stock : rootStocks) {
        rows.append(createAuditRow(stock, machine.rootStorageId));

        zInfo(L("Talált készlet [%1], mennyiség: %2")
                  .arg(stock.materialName())
                  .arg(stock.quantity));
    }

    // tároló gyökérelem alatti tárolók tartalmának a kigyűjtése
    for (const auto& storage : storageEntries) {
        const auto stocks = stockByStorage.values(storage.id);
        for (const auto& stock : stocks) {
            rows.append(createAuditRow(stock, machine.rootStorageId));

            zInfo(L("Talált készlet [%1] a tárolóban [%2], mennyiség: %3")
                                   .arg(stock.materialName())
                                   .arg(storage.name)
                                   .arg(stock.quantity));
        }
    }

    return rows;
}

StorageAuditRow StorageAuditService::createAuditRow(
    const StockEntry& stock,
    const QUuid& rootStorageId)
{

    if (stock.materialName().isEmpty() && stock.quantity == 0)
        return {}; // vagy dobj vissza egy optional/null bejegyzést


    StorageAuditRow row;
    row.rowId           = QUuid::createUuid();         // egyedi audit sor azonosító
    row.materialId      = stock.materialId;            // Anyag azonosító
    row.stockEntryId    = stock.entryId;               // 🔗 kapcsolat a StockEntry-hez
    row.sourceType = AuditSourceType::Stock;

    //row.pickingQuantity = 0;//pickingMap.value(stock.materialBarcode(), 0); // vagy materialName alapján
    row.actualQuantity   = stock.quantity;
    row.originalQuantity = stock.quantity;
    //row.rowPresence = AuditPresence::Unknown;         // Felhasználó fogja beállítani
    row.isInOptimization = false;                  // Később validálható CutPlan alapján

    row.barcode = stock.materialBarcode();
    row.storageName = stock.storageName();
    row.rootStorageId   = rootStorageId; // 🆕 itt állítjuk be

    return row;
}

