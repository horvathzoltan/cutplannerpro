#pragma once

#include <model/movementdata.h>

#include <model/registries/materialregistry.h>
#include <model/registries/stockregistry.h>
#include <model/registries/storageregistry.h>


namespace MovementDataHelper {

inline MovementData_Log fromMovementData(const MovementData& data)  {
    MovementData_Log log;

    // Resolve item name/barcode: prefer explicit itemId, else try fromEntryId, else try toEntryId
    QUuid itemId = data.materialId;
    // if (itemId.isNull() && !data.fromEntryId.isNull()) {
    //     if (auto src = StockRegistry::instance().findById(data.fromEntryId))
    //         itemId = src->materialId;
    // }
    // if (itemId.isNull() && !data.toEntryId.isNull()) {
    //     if (auto tgt = StockRegistry::instance().findById(data.toEntryId))
    //         itemId = tgt->materialId;
    // }
    if (!itemId.isNull()) {
        if (auto mat = MaterialRegistry::instance().findById(itemId)) {
            log.itemName = mat->name;
            log.itemBarcode = mat->barcode;
        }
    }

    // Resolve from storage name/barcode from fromEntryId (if present)
    if (!data.fromEntryId.isNull()) {
        if (auto src = StockRegistry::instance().findById(data.fromEntryId)) {
            if (auto s = StorageRegistry::instance().findById(src->storageId)) {
                log.fromStorageName = s->name;
                log.fromBarcode = s->barcode;
            }
        }
    }

    // Resolve to storage name/barcode:
    // prefer explicit toEntryId (gives canonical storage), otherwise use toStorageId
    if (!data.toEntryId.isNull()) {
        if (auto tgt = StockRegistry::instance().findById(data.toEntryId)) {
            if (auto s = StorageRegistry::instance().findById(tgt->storageId)) {
                log.toStorageName = s->name;
                log.toBarcode = s->barcode;
            }
        }
    } else if (!data.toStorageId.isNull()) {
        if (auto s = StorageRegistry::instance().findById(data.toStorageId)) {
            log.toStorageName = s->name;
            log.toBarcode = s->barcode;
        }
    }

    return log;
}

}
