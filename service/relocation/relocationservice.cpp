#include "relocationservice.h"
#include "service/stockmovementservice.h"
#include "model/movementdata.h"
#include "model/registries/stockregistry.h"

bool RelocationService::finalize(RelocationInstruction& instr, CuttingPresenter* presenter) {
    if (!instr.isReadyToFinalize() || instr.isAlreadyFinalized())
        return false;

    // 1️⃣ Forrásokból levonás
    for (const auto& src : instr.sources) {
        if (src.moved <= 0) continue;

        auto opt = StockRegistry::instance().findById(src.entryId);
        if (!opt) continue; // ha közben törölték, skip
        const StockEntry& original = *opt;

        MovementData md;
        md.quantity = src.moved;
        md.comment  = QStringLiteral("Relocation finalize (source)");

        // csak forrás → levonás
        StockMovementService::moveStock(original, md, presenter);
    }

    // 2️⃣ Célokra felírás
    for (const auto& tgt : instr.targets) {
        if (tgt.placed <= 0) continue;

        // dummy StockEntry a materialId miatt
        StockEntry dummy;
        dummy.materialId = instr.materialId;

        MovementData md;
        md.toStorageId = tgt.locationId;
        md.quantity    = tgt.placed;
        md.comment     = QStringLiteral("Relocation finalize (target)");

        // csak cél → hozzáadás
        StockMovementService::moveStock(dummy, md, presenter);
    }

    // 3️⃣ Instr frissítése
    instr.executedQuantity = instr.plannedQuantity;
    instr.isFinalized = true;
    return true;
}
