#include "storageauditservice.h"
#include "model/registries/cuttingmachineregistry.h"
#include "model/registries/storageregistry.h"
#include "model/registries/stockregistry.h"
#include "common/logger.h"
#include "model/storageaudit/storageauditrow.h"

#include <QMultiMap>

StorageAuditService::AuditMode StorageAuditService::_mode = AuditMode::Passive;

StorageAuditService::StorageAuditService(QObject* parent)
    : QObject(parent) {}

QVector<StorageAuditRow> StorageAuditService::generateAuditRows(const QMap<QString, int>& pickingMap)
{
    QVector<StorageAuditRow> result;

    const auto& machines = CuttingMachineRegistry::instance().readAll();
    zInfo(L("Gépek száma: %1").arg(machines.size()));

    for (const auto& machine : machines) {
        zInfo(L("Auditálás géphez: %1, rootStorageId: %2")
                  .arg(machine.name)
                  .arg(machine.rootStorageId.toString()));

        auto machineEntries = auditMachineStorage(machine, pickingMap);

        zInfo(L("Audit bejegyzések száma géphez [%1]: %2")
                  .arg(machine.name)
                  .arg(machineEntries.size()));

        result += machineEntries;
    }

    zInfo(L("Összes audit bejegyzés: %1").arg(result.size()));
    return result;
}

QVector<StorageAuditRow> StorageAuditService::auditMachineStorage(const CuttingMachine& machine, const QMap<QString, int>& pickingMap)
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

    const auto rootStocks = stockByStorage.values(machine.rootStorageId);
    for (const auto& stock : rootStocks) {
        rows.append(createAuditRow(stock, machine.name + " – műhely zóna", pickingMap));

        zInfo(L("Talált készlet [%1], mennyiség: %2")
                  .arg(stock.materialName())
                  .arg(stock.quantity));

    }

    for (const auto& storage : storageEntries) {
        const auto stocks = stockByStorage.values(storage.id);
        for (const auto& stock : stocks) {
            rows.append(createAuditRow(stock, storage.name, pickingMap));

            zInfo(L("Talált készlet [%1] a tárolóban [%2], mennyiség: %3")
                                   .arg(stock.materialName())
                                   .arg(storage.name)
                                   .arg(stock.quantity));
        }
    }

    return rows;
}

StorageAuditRow StorageAuditService::createAuditRow(const StockEntry& stock, const QString& storageName, const QMap<QString, int>& pickingMap)
{

    if (stock.materialName().isEmpty() && stock.quantity == 0)
        return {}; // vagy dobj vissza egy optional/null bejegyzést


    StorageAuditRow row;
    row.materialName     = stock.materialName();
    row.storageName      = storageName;
    //entry.expectedQuantity = 0;
    row.actualQuantity   = stock.quantity;
    row.isPresent        = stock.quantity > 0;
    //entry.missingQuantity  = std::max(0, entry.expectedQuantity - entry.actualQuantity);

    row.pickingQuantity = pickingMap.value(stock.materialBarcode(), 0); // vagy materialName alapján

    // if (_mode == AuditMode::Passive) {
    //     row.expectedQuantity = 0;
    //     row.missingQuantity  = 0;
    // } else {
    //     row.expectedQuantity = 1; // vagy a picking list alapján
    //     row.missingQuantity  = std::max(0, row.expectedQuantity - stock.quantity);
    // }

    return row;
}

