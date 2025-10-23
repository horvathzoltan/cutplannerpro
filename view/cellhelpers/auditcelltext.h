#pragma once
#include "common/logger.h"
// #include "model/storageaudit/auditcontext.h"
// #include "model/material/materialmaster.h"
#include "model/storageaudit/storageauditrow.h"
#include <QString>

namespace AuditCellText {

inline static bool _isVerbose = false;

/**
 * @brief Elvárt mennyiség szöveges formázása audit sorhoz.
 *
 * Logika:
 * - Hulló audit sor esetén: ha optimalizációban van és van pickingQuantity → "1 db", különben "–".
 * - Csoportosított stock sor esetén:
 *    - Ha nincs optimalizáció → "– (anyagcsoport [címke])"
 *    - Ha van optimalizáció → "<összesített db> (anyagcsoport [címke])"
 * - Egyedi stock sor esetén:
 *    - Ha van optimalizáció → "<lokális db>"
 *    - Ha nincs optimalizáció → "–"
 *
 * @param row        Audit sor
 * @param groupLabel Csoportcímke (pl. "A", "B", stb.)
 * @return QString   Formázott szöveg
 */
inline QString forExpected(const StorageAuditRow& row, const QString& groupLabel = "")
{
    // 🧩 Hulló audit sor
    if (row.sourceType == AuditSourceType::Leftover) {
        if (row.isInOptimization && row.pickingQuantity > 0)
            return QString("%1 db (hulló)").arg(row.pickingQuantity);
        return "–";
    }

    // 🧩 Csoportosított stock sor
    if (row.context && row.context->group.isGroup()) {
        if (!row.isInOptimization) {
            return groupLabel.isEmpty()
            ? "– (anyagcsoport)"
            : QString("– (anyagcsoport %1)").arg(groupLabel);
        }
        int expected = row.context->totalExpected;
        return groupLabel.isEmpty()
                   ? QString("%1 db (anyagcsoport)").arg(expected)
                   : QString("%1 db (anyagcsoport %2)").arg(expected).arg(groupLabel);
    }

    // 🔹 Egyedi stock sor
    if (row.isInOptimization && row.pickingQuantity > 0)
        return QString("%1 db").arg(row.pickingQuantity);

    return "–";
}


/**
 * @brief Hiányzó mennyiség szöveges formázása audit sorhoz.
 *
 * Logika:
 * - Hulló audit sor esetén: ha van elvárás → lokális hiány, különben "–".
 * - Csoportosított stock sor esetén: aggregált hiány, de nem írjuk ki újra a csoportcímkét.
 * - Egyedi stock sor esetén: lokális hiány.
 * - Ha nincs optimalizáció → "–".
 */
inline QString forMissing(const StorageAuditRow& row) {
    // 🧩 Hulló audit sor
    if (row.sourceType == AuditSourceType::Leftover) {
        if (row.isInOptimization && row.pickingQuantity > 0) {
            return QString("%1 db").arg(std::max(0, row.missingQuantity()));
        }
        return "–";
    }

    // 🧩 Csoportosított stock sor
    if (row.context && row.context->group.isGroup()) {
        if (!row.isInOptimization)
            return "–";
        int missing = std::max(0, row.context->totalExpected - row.context->totalActual);
        return QString("%1 db").arg(missing);
    }

    // 🔹 Egyedi stock sor
    return row.isInOptimization
               ? QString("%1 db").arg(std::max(0, row.missingQuantity()))
               : "–";
}

}
