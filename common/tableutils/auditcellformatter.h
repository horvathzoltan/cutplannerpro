#pragma once

#include "model/storageaudit/storageauditrow.h"
#include <QString>

namespace AuditCellFormatter {

/**
 * @brief Elvárt mennyiség szöveges formázása audit sorhoz.
 *
 * Logika:
 * - Hulló audit soroknál nincs elvárt mennyiség → visszatér üresen.
 * - Ha a sor csoport tagja, és van elvárt mennyiség → csoport szintű érték jelenik meg.
 * - Egyedi sor esetén a lokális pickingQuantity jelenik meg.
 * - Ha nincs elvárt mennyiség → visszatér üresen.
 */
inline QString formatExpectedQuantity(const StorageAuditRow& row) {
    // 🧩 Hulló audit sor esetén: csak akkor jelenítsünk meg elvárást, ha tényleg van
    if (row.sourceType == AuditSourceType::Leftover) {
        if (row.isInOptimization && row.pickingQuantity > 0)
            return QString("%1 db").arg(row.pickingQuantity); // pl. "1 db"
        return "–"; // nincs elvárás → vizuálisan semleges
    }

    // 🧩 Csoportosított stock sor esetén: aggregált elvárt mennyiség
    if (row.context && row.context->group.rowIds.size() > 1) {
        int expected = row.context->group.totalExpected;
        return expected > 0
                   ? QString("%1 db (anyagcsoport)").arg(expected)
                   : "0 db";
    }

    // 🔹 Egyedi stock sor esetén: lokális elvárt mennyiség
    return row.pickingQuantity > 0
               ? QString("%1 db").arg(row.pickingQuantity)
               : "0 db";
}

/**
 * @brief Hiányzó mennyiség szöveges formázása audit sorhoz.
 *
 * Logika:
 * - Hulló audit soroknál nincs hiány → visszatér üresen.
 * - Ha a sor csoport tagja, és van hiány → csoport szintű hiány jelenik meg.
 * - Egyedi sor esetén a lokális hiányzó mennyiség jelenik meg.
 * - Ha nincs elvárt mennyiség → visszatér üresen.
 */
/**
 * @brief Hiányzó mennyiség szöveges formázása audit sorhoz.
 *
 * Logika:
 * - Hulló audit sor esetén: ha van elvárt mennyiség, akkor lokális hiányt mutat.
 * - Csoportosított stock sor esetén: aggregált hiányt mutat.
 * - Egyedi stock sor esetén: lokális hiányt mutat.
 * - Ha nincs hiány, akkor "0 db" jelenik meg → vizuálisan is megerősíti a teljesülést.
 */
inline QString formatMissingQuantity(const StorageAuditRow& row) {
    // 🧩 Hulló audit sor: ha van elvárás, akkor számolunk hiányt
    if (row.sourceType == AuditSourceType::Leftover) {
        if (row.pickingQuantity > 0) {
            int missing = row.missingQuantity();
            return QString("%1 db").arg(missing); // lehet 0 is
        }
        return "–"; // nincs elvárás → semleges megjelenítés
    }

    // 🧩 Csoportosított stock sor esetén: aggregált hiány
    if (row.context && row.context->group.rowIds.size() > 1) {
        int missing = row.context->group.totalExpected - row.context->group.totalActual;
        return QString("%1 db (anyagcsoport)").arg(missing); // lehet 0 is
    }

    // 🔹 Egyedi stock sor esetén: lokális hiány
    return QString("%1 db").arg(row.missingQuantity()); // lehet 0 is
}

} // namespace AuditCellFormatter
