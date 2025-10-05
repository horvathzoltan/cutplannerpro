#pragma once

#include "model/relocation/relocationinstruction.h"
#include "model/relocation/relocationquantityrow.h"
#include <QVector>

namespace RelocationQuantityHelpers {

/**
 * @brief RelocationInstruction → dialógus sorok (forrás + cél tárhelyek).
 *
 * A dialógus megnyitásakor ezzel generáljuk a szerkeszthető sorokat.
 */
inline QVector<RelocationQuantityRow> generateQuantityRows(const RelocationInstruction& instruction) {
    QVector<RelocationQuantityRow> rows;

    for (const auto& src : instruction.sources) {
        RelocationQuantityRow r;
        r.storageName = src.locationName;
        r.available = src.available;
        r.selected = src.moved;
        r.isTarget = false;
        rows.append(r);
    }

    for (const auto& tgt : instruction.targets) {
        RelocationQuantityRow r;
        r.storageName = tgt.locationName;
        r.current = 0; // opcionálisan bővíthető
        r.selected = tgt.placed;
        r.isTarget = true;
        rows.append(r);
    }

    return rows;
}

/**
 * @brief Dialógus sorok → RelocationInstruction (visszatöltés).
 *
 * A dialógus bezárásakor ezzel frissítjük az instruction-t.
 */
inline void applyQuantityRows(RelocationInstruction& instruction, const QVector<RelocationQuantityRow>& rows) {
    instruction.sources.clear();
    instruction.targets.clear();

    int totalMoved = 0;

    for (const auto& r : rows) {
        if (r.isTarget) {
            RelocationTargetEntry tgt;
            tgt.locationName = r.storageName;
            tgt.placed = r.selected;
            instruction.targets.append(tgt);
        } else {
            RelocationSourceEntry src;
            src.locationName = r.storageName;
            src.available = r.available;
            src.moved = r.selected;
            instruction.sources.append(src);
            totalMoved += r.selected;
        }
    }

    instruction.executedQuantity = totalMoved;
    instruction.isFinalized = false; // csak finalize után lesz true
}

inline QVector<RelocationQuantityRow> generateSourceRows(const RelocationInstruction& instruction) {
    QVector<RelocationQuantityRow> rows;

    for (const auto& src : instruction.sources) {
        RelocationQuantityRow r;
        r.storageName = src.locationName;
        r.available   = src.available;
        r.selected    = src.moved;
        r.isTarget    = false;
        rows.append(r);
    }

    return rows;
}

inline QVector<RelocationQuantityRow> generateTargetRows(const RelocationInstruction& instruction) {
    QVector<RelocationQuantityRow> rows;

    for (const auto& tgt : instruction.targets) {
        RelocationQuantityRow r;
        r.storageName = tgt.locationName;
        r.current     = 0; // opcionálisan bővíthető
        r.selected    = tgt.placed;
        r.isTarget    = true;
        rows.append(r);
    }

    return rows;
}

inline void applySourceRows(RelocationInstruction& instruction,
                            const QVector<RelocationQuantityRow>& rows) {
    instruction.sources.clear();
    int totalMoved = 0;

    for (const auto& r : rows) {
        RelocationSourceEntry src;
        src.locationName = r.storageName;
        src.available    = r.available;
        src.moved        = r.selected;
        instruction.sources.append(src);
        totalMoved += r.selected;
    }

    instruction.executedQuantity = totalMoved;
    instruction.isFinalized = false;
}

inline void applyTargetRows(RelocationInstruction& instruction,
                            const QVector<RelocationQuantityRow>& rows) {
    instruction.targets.clear();

    for (const auto& r : rows) {
        RelocationTargetEntry tgt;
        tgt.locationName = r.storageName;
        tgt.placed       = r.selected;
        instruction.targets.append(tgt);
    }

    instruction.isFinalized = false;
}

} // namespace RelocationQuantityHelpers
