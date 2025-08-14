#include "presenter/CuttingPresenter.h"
#include <model/registries/stockregistry.h>
#include <model/registries/storageregistry.h>
#include "movementlogger.h"     // MovementLogger::log
#include "model/movementlogmodel.h"    // MovementLogModel

class StockMovementService {
public:
    explicit StockMovementService(CuttingPresenter* presenter)
        : presenter_(presenter) {}

    bool move(const QUuid& originalId,
              const QUuid& toStorageId,
              int qty,
              const QString& comment)
    {
        // 1) Betöltés
        auto opt = StockRegistry::instance().findById(originalId);
        if (!opt) return false;
        const StockEntry original = *opt;

        // 2) Validálás
        if (qty <= 0 || qty > original.quantity) return false;
        if (toStorageId.isNull()) return false;

        const auto* srcStorage = StorageRegistry::instance().findById(original.storageId);
        const auto* dstStorage = StorageRegistry::instance().findById(toStorageId);
        if (!dstStorage) return false;
        if (srcStorage && (dstStorage->id == srcStorage->id)) return false;

        // 3) Új bejegyzés összeállítás
        StockEntry moved = original;
        moved.entryId = QUuid::createUuid();
        moved.storageId = toStorageId;
        moved.quantity = qty;
        moved.comment = comment;

        const int remaining = original.quantity - qty;

        // 4) Írási lépések (egyszerű sorrenddel)
        if (remaining > 0) {
            StockEntry updated = original;
            updated.quantity = remaining;
            presenter_->update_StockEntry(updated);
            presenter_->add_StockEntry(moved);
        } else {
            presenter_->add_StockEntry(moved);
            presenter_->remove_StockEntry(original.entryId);
        }

        // 5) Log – csak sikeres állapotváltozás után
        MovementData md;
        md.fromEntryId = original.entryId;
        md.toStorageId = toStorageId;
        md.quantity = qty;
        md.comment = comment;

        // Enrichment a loghoz (ha a MovementData-t kibővítetted ezekkel a mezőkkel)
        if (srcStorage) {
            md.fromStorageName = srcStorage->name;
            md.fromBarcode = srcStorage->barcode;
        }
        if (dstStorage) {
            md.toStorageName = dstStorage->name;
            md.toBarcode = dstStorage->barcode;
        }
        // Ha van termék entitásod, itt töltsd: md.itemName, md.itemBarcode

        MovementLogger::log(MovementLogModel{md});
        return true;
    }

private:
    CuttingPresenter* presenter_ = nullptr;
};
