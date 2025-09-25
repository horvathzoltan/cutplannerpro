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
 * - Ha a sor valódi csoport tagja (group.isGroup()), akkor a context aggregált értékét mutatjuk.
 * - Egyedi sor esetén a lokális pickingQuantity jelenik meg.
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
              .arg(row.context ? row.context->group.size() : -1)
              .arg(groupLabel));

    // 🧩 Hulló audit sor esetén: csak akkor jelenítsünk meg elvárást, ha tényleg van
    if (row.sourceType == AuditSourceType::Leftover) {
        if (row.isInOptimization && row.pickingQuantity > 0)
            return QString("%1 db").arg(row.pickingQuantity); // pl. "1 db"
        return "–"; // nincs elvárás → vizuálisan semleges
    }

    // 🧩 Csoportosított stock sor esetén: aggregált elvárt mennyiség
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
    if (row.context && row.context->group.isGroup()) {
        if (!row.isInOptimization)
            return "– (anyagcsoport)";
        int missing = std::max(0, row.context->totalExpected - row.context->totalActual);
        return QString("%1 db (anyagcsoport)").arg(missing);
    }


    // 🔹 Egyedi stock sor esetén: lokális hiány
    return row.isInOptimization
               ? QString("%1 db").arg(std::max(0, row.missingQuantity()))
               : "–";
}


} // namespace AuditCellFormatter
