#pragma once

#include "../../model/relocation/relocationinstruction.h"
#include "../../model/relocation/relocationquantityrow.h"
#include <QVector>

namespace RelocationQuantityHelpers {

/*SOURCE*/

inline QVector<RelocationQuantityRow> generateSourceRows(const RelocationInstruction& instruction) {
    QVector<RelocationQuantityRow> rows;

    for (const auto& src : instruction.sources) {
        RelocationQuantityRow r;

        // kritikus identitások:
        r.entryId     = src.entryId;       // konkrét stock entry
        r.storageId   = src.locationId;     // 🔹 forrás tárhely UUID

        // további mezők:
        r.storageName = src.locationName;
        r.available   = src.available;
        r.selected    = src.moved;
        r.isTarget    = false;

        rows.append(r);
    }

    return rows;
}

inline void applySourceRows(RelocationInstruction& instruction,
                            const QVector<RelocationQuantityRow>& rows) {
    instruction.sources.clear();
    //int totalMoved = 0;

    for (const auto& r : rows) {
        RelocationSourceEntry src;
        // ezek kritikusak:
        src.entryId      = r.entryId;      // ✅ forrás entry azonosító
        src.locationId   = r.storageId;    // ✅ forrás storage azonosító

        // kiegészítők:
        src.locationName = r.storageName;
        src.available    = r.available;
        src.moved        = r.selected;

        instruction.sources.append(src);
        //totalMoved += r.selected;
    }

    // A totalMovedQuantity itt a DIALÓGUS összesített értéke (átmeneti).
    // Nem perzisztáljuk vele a véglegesített mennyiséget.
    //instruction.dialogTotalMovedQuantity = totalMoved;
    // Az alkalmazás pillanatában ne írjuk felül a véglegesített mezőt.
    // finalizedQuantity legyen nullopt, ha még nincs finalize‑olt érték.
    instruction.finalizedQuantity = std::nullopt; // csak a finalize során kerül beállításra

    // isFinalized tükrözheti a perzisztált állapotot; dialog apply során maradjon false,
    // a tényleges finalizeRelocation állítsa be szükség szerint.
    //instruction.isFinalized = false;

}


/*TARGET*/

inline QVector<RelocationQuantityRow> generateTargetRows(const RelocationInstruction& instruction) {
    QVector<RelocationQuantityRow> rows;

    for (const auto& tgt : instruction.targets) {
        RelocationQuantityRow r;
        r.storageId   = tgt.locationId;     // 🔹 cél tárhely UUID
        r.storageName = tgt.locationName;
        r.current     = 0; // opcionálisan bővíthető
        r.selected    = tgt.placed;
        r.isTarget    = true;
        rows.append(r);
    }

    return rows;
}

inline void applyTargetRows(RelocationInstruction& instruction,
                            const QVector<RelocationQuantityRow>& rows) {
    instruction.targets.clear();

    for (const auto& r : rows) {
        RelocationTargetEntry tgt;
        tgt.locationName = r.storageName;
        tgt.locationId   = r.storageId;   // 🔹 most már átadjuk a storageId-t is
        tgt.placed       = r.selected;
        instruction.targets.append(tgt);
    }

   // instruction.isFinalized = false;
}

} // namespace RelocationQuantityHelpers
