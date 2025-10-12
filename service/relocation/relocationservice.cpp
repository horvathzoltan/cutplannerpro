#include "relocationservice.h"
#include "service/stockmovementservice.h"
#include "model/movementdata.h"
#include "model/registries/stockregistry.h"

bool RelocationService::finalize(RelocationInstruction& instr, CuttingPresenter* presenter) {
    zTrace();
    if (!instr.isReadyToFinalize() || instr.isAlreadyFinalized()) {
        zInfo("finalize: instruction not ready or already finalized");
        return false;
    }

    StockRegistry::instance().dumpAll(); // debug snapshot

    // 1️⃣ Forrásokból levonás (entry-szintű moveStock hívások)
    for (const auto& src : instr.sources) {
        if (src.moved <= 0) continue;

        MovementData md;
        md.quantity = src.moved;
        md.comment = QStringLiteral("Relocation finalize (source)");

        // Ha van explicit entryId, használjuk
        if (!src.entryId.isNull()) {
            md.fromEntryId = src.entryId;
            // Próbáljuk lekérni, és beállítani az itemId a loghoz / célfeloldáshoz
            if (auto opt = StockRegistry::instance().findById(src.entryId)) {
                md.materialId = opt->materialId;
            } else {
                zWarning(QStringLiteral("finalize: source entryId not found: %1 — will try resolve by storage")
                             .arg(src.entryId.toString()));
            }
        }

        // Ha nincs entryId vagy nem található, próbáljuk resolve-olni storage+material alapján
        if (md.fromEntryId.isNull()) {
            if (!src.locationId.isNull()) {
                auto opt2 = StockRegistry::instance().findFirstByStorageAndMaterial(src.locationId, instr.materialId);
                if (opt2) {
                    md.fromEntryId = opt2->entryId;
                    md.materialId = opt2->materialId;
                    zInfo(QStringLiteral("finalize: resolved source entry from storage=%1 -> entry=%2")
                              .arg(src.locationId.toString(), md.fromEntryId.toString()));
                } else {
                    zWarning(QStringLiteral("finalize: cannot resolve source entry for storage=%1 material=%2; skipping")
                                 .arg(src.locationId.toString(), instr.materialId.toString()));
                    continue;
                }
            } else {
                zWarning("finalize: source has no entryId and no locationId; skipping");
                continue;
            }
        }

        zInfo(QStringLiteral("finalize: calling moveStock (source) fromEntry=%1 qty=%2")
                  .arg(md.fromEntryId.toString()).arg(md.quantity));

        if (!StockMovementService::moveStock(md, presenter)) {
            zWarning(QStringLiteral("finalize: moveStock failed for source entry=%1 qty=%2")
                         .arg(md.fromEntryId.toString()).arg(md.quantity));
            return false;
        }
    }

    // 2️⃣ Célokra felírás (deposit / aggregate)
    for (const auto& tgt : instr.targets) {
        if (tgt.placed <= 0) continue;

        MovementData md;
        md.materialId = instr.materialId;     // material reference is required for aggregation/creation
        md.toStorageId = tgt.locationId;
        md.quantity = tgt.placed;
        md.comment = QStringLiteral("Relocation finalize (target)");

        zInfo(QStringLiteral("finalize: calling moveStock (target) toStorage=%1 qty=%2")
                  .arg(md.toStorageId.toString()).arg(md.quantity));

        if (!StockMovementService::moveStock(md, presenter)) {
            zWarning(QStringLiteral("finalize: moveStock failed for target storage=%1 qty=%2")
                         .arg(md.toStorageId.toString()).arg(md.quantity));
            return false;
        }
    }

    // 3️⃣ Instr frissítése
    instr.executedQuantity = instr.plannedQuantity;
    instr.isFinalized = true;

    StockRegistry::instance().dumpAll(); // debug snapshot after ops
    zInfo("finalize: relocation succeeded");
    return true;
}

