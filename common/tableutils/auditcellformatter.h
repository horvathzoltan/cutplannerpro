#pragma once

#include "common/logger.h"
#include "model/storageaudit/storageauditrow.h"
#include <QString>

namespace AuditCellFormatter {

/**
 * @brief Elvárt mennyiség szöveges formázása audit sorhoz.
 *
 * Logika:
 * - Hulló audit soroknál nincs elvárt mennyiség → visszatér "–".
 * - Ha a sor csoport tagja, és van elvárt mennyiség → csoport szintű érték jelenik meg.
 * - Egyedi sor esetén a lokális pickingQuantity jelenik meg.
 * - Ha nincs elvárt mennyiség → visszatér "–".
 * - Ha nincs optimalizáció → visszatér "–" (vizuálisan semleges).
 * - Ha van csoportazonosító (groupLabel), megjelenik az érték mellett.
 */
inline QString formatExpectedQuantity(const StorageAuditRow& row, const QString& groupLabel = "") {
    zInfo(L("📦 formatExpectedQuantity: rowId=%1, materialId=%2, sourceType=%3, isInOptimization=%4, pickingQuantity=%5, context=%6, groupSize=%7, groupLabel=%8")
              .arg(row.rowId.toString())
              .arg(row.materialId.toString())
              .arg(static_cast<int>(row.sourceType))
              .arg(row.isInOptimization)
              .arg(row.pickingQuantity)
              .arg(row.context ? "yes" : "no")
              .arg(row.context ? row.context->group.rowIds.size() : -1)
              .arg(groupLabel));



    // 🧩 Hulló audit sor esetén: csak akkor jelenítsünk meg elvárást, ha tényleg van
    if (row.sourceType == AuditSourceType::Leftover) {
        if (row.isInOptimization && row.pickingQuantity > 0)
            return QString("%1 db").arg(row.pickingQuantity); // pl. "1 db"
        return "–"; // nincs elvárás → vizuálisan semleges
    }

    // 🧩 Csoportosított stock sor esetén: aggregált elvárt mennyiség
    if (row.context){// && row.context->group.rowIds.size() > 1) {
        int expected = row.context->group.totalExpected;
        if (!row.isInOptimization)
            return "–"; // nincs optimalizáció → nincs elvárt

        return groupLabel.isEmpty()
                   ? QString("%1 db (anyagcsoport)").arg(expected)
                   : QString("%1 db (anyagcsoport %2)").arg(expected).arg(groupLabel);
    }

    // 🔹 Egyedi stock sor esetén: lokális elvárt mennyiség
    return row.isInOptimization
               ? QString("%1 db").arg(row.pickingQuantity)
               : "–";
}


/**
 * @brief Hiányzó mennyiség szöveges formázása audit sorhoz.
 *
 * Logika:
 * - Hulló audit sor esetén: ha van elvárt mennyiség, akkor lokális hiányt mutat.
 * - Csoportosított stock sor esetén: aggregált hiányt mutat.
 * - Egyedi stock sor esetén: lokális hiányt mutat.
 * - Ha nincs hiány, akkor "0 db" jelenik meg → vizuálisan is megerősíti a teljesülést.
 * - Ha nincs optimalizáció → visszatér "–".
 * - A hiány sosem lehet negatív → max(0, expected - actual).
 */
inline QString formatMissingQuantity(const StorageAuditRow& row) {
    // 🧩 Hulló audit sor: ha van elvárás, akkor számolunk hiányt
    if (row.sourceType == AuditSourceType::Leftover) {
        if (row.isInOptimization && row.pickingQuantity > 0) {
            int missing = std::max(0, row.missingQuantity());
            return QString("%1 db").arg(missing); // lehet 0 is
        }
        return "–"; // nincs elvárás → semleges megjelenítés
    }

    // 🧩 Csoportosított stock sor esetén: aggregált hiány
    if (row.context && row.context->group.rowIds.size() > 1) {
        if (!row.isInOptimization)
            return "–"; // nincs optimalizáció → nincs hiány

        int missing = std::max(0, row.context->group.totalExpected - row.context->group.totalActual);
        return QString("%1 db (anyagcsoport)").arg(missing); // lehet 0 is
    }

    // 🔹 Egyedi stock sor esetén: lokális hiány
    return row.isInOptimization
               ? QString("%1 db").arg(std::max(0, row.missingQuantity()))
               : "–";
}

} // namespace AuditCellFormatter
