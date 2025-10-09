#include "presenter/CuttingPresenter.h"
#include <model/registries/materialregistry.h>
#include <model/registries/stockregistry.h>
#include <model/registries/storageregistry.h>
#include "movementlogger.h"     // MovementLogger::log
//#include "model/movementlogmodel.h"    // MovementLogModel

class StockMovementService {
public:
    explicit StockMovementService(CuttingPresenter* presenter)
        : presenter_(presenter) {}

    // Convenience wrapper: ID alapján betölt, majd moveStock hívás
    bool move(const QUuid& originalId,
              const QUuid& toStorageId,
              int qty,
              const QString& comment)
    {
        auto opt = StockRegistry::instance().findById(originalId);
        if (!opt) return false;

        MovementData md;
        md.fromEntryId = originalId;   // 🔹 logban is látszik a forrás
        md.toStorageId = toStorageId;
        md.quantity    = qty;
        md.comment     = comment;

        return moveStock(*opt, md, presenter_);
    }

    // Relocation finalize: minden forrás és cél a moveStock()-on keresztül megy
    bool finalizeRelocation(RelocationInstruction& instr)
    {
        if (!instr.isReadyToFinalize() || instr.isAlreadyFinalized())
            return false;

        // Forrásokból levonás
        for (const auto& src : instr.sources) {
            if (src.moved <= 0) continue;

            auto opt = StockRegistry::instance().findById(src.entryId);
            if (!opt) continue;
            const StockEntry& original = *opt;

            MovementData md;
            md.fromEntryId = src.entryId;   // 🔹 pontos forrás
            md.quantity    = src.moved;
            md.comment     = QStringLiteral("Relocation: source");

            moveStock(original, md, presenter_);
        }

        // Célokra felírás
        for (const auto& tgt : instr.targets) {
            if (tgt.placed <= 0) continue;

            StockEntry dummy;
            dummy.materialId = instr.materialId;

            MovementData md;
            md.toStorageId = tgt.locationId; // 🔹 pontos cél
            md.quantity    = tgt.placed;
            md.comment     = QStringLiteral("Relocation: target");

            moveStock(dummy, md, presenter_);
        }

        // Instr frissítése
        instr.executedQuantity = instr.plannedQuantity;
        instr.isFinalized = true;
        return true;
    }

    static bool moveStock(const StockEntry& original,
                          const MovementData& data,
                          CuttingPresenter* presenter)
    {
        // 1) Validáció – rugalmasabb, hogy relocation esetet is kezelje
        if (data.quantity <= 0) return false;

        const bool hasSource = !original.entryId.isNull();   // van forrás bejegyzés
        const bool hasTarget = !data.toStorageId.isNull();   // van cél tárhely

        if (!hasSource && !hasTarget) return false; // semmi értelme

        const auto* srcStorage = hasSource ? StorageRegistry::instance().findById(original.storageId) : nullptr;
        const auto* dstStorage = hasTarget ? StorageRegistry::instance().findById(data.toStorageId) : nullptr;

        // Ha klasszikus mozgatás van, ne engedjük ugyanabba a tárhelybe
        if (hasSource && hasTarget && srcStorage && dstStorage && srcStorage->id == dstStorage->id)
            return false;

        // 2) Registry módosítás
        if (hasSource && hasTarget) {
            // 🔹 Klasszikus mozgatás: forrásból levonás + célra új bejegyzés
            if (data.quantity > original.quantity) return false;

            StockEntry movedEntry = original;
            movedEntry.entryId   = QUuid::createUuid();
            movedEntry.storageId = data.toStorageId;
            movedEntry.quantity  = data.quantity;
            movedEntry.comment   = data.comment;

            int remaining = original.quantity - data.quantity;
            if (remaining > 0) {
                StockEntry updated = original;
                updated.quantity = remaining;
                presenter->update_StockEntry(updated);
                presenter->add_StockEntry(movedEntry);
            } else {
                presenter->add_StockEntry(movedEntry);
                presenter->remove_StockEntry(original.entryId);
            }
        }
        else if (hasSource) {
            // 🔹 Csak levonás (consume): relocation forrásból kivonás
            if (data.quantity > original.quantity) return false;

            int remaining = original.quantity - data.quantity;
            if (remaining > 0) {
                StockEntry updated = original;
                updated.quantity = remaining;
                presenter->update_StockEntry(updated);
            } else {
                presenter->remove_StockEntry(original.entryId);
            }
        }
        else if (hasTarget) {
            // 🔹 Csak hozzáadás (add): relocation célra felírás
            StockEntry newEntry;
            newEntry.entryId    = QUuid::createUuid();
            newEntry.storageId  = data.toStorageId;
            newEntry.materialId = original.materialId; // fontos: anyag azonosító öröklése
            newEntry.quantity   = data.quantity;
            newEntry.comment    = data.comment;

            presenter->add_StockEntry(newEntry);
        }

        // 3) Logolás – minden esetben
        MovementData md = data;
        md.fromEntryId = hasSource ? original.entryId : QUuid();

        if (srcStorage) {
            md.fromStorageName = srcStorage->name;
            md.fromBarcode     = srcStorage->barcode;
        }
        if (dstStorage) {
            md.toStorageName = dstStorage->name;
            md.toBarcode     = dstStorage->barcode;
        }
        if (const auto* item = MaterialRegistry::instance().findById(original.materialId)) {
            md.itemName    = item->name;
            md.itemBarcode = item->barcode;
        }

        MovementLogger::log(MovementLogModel{md});
        return true;
    }


private:
    CuttingPresenter* presenter_ = nullptr;
};
