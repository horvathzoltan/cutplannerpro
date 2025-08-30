#pragma once

#include "common/grouputils.h"
#include "common/tablerowstyler/materialrowstyler.h"
#include "model/material/materialmaster.h"
#include "model/storageaudit/storageauditrow.h"
#include <QTableWidget>



namespace ColorLogicUtils{
inline QColor resolveBaseColor(const MaterialMaster* mat) {
    if (!mat) return QColor(Qt::lightGray);
    return GroupUtils::colorForGroup(mat->id); // vagy MaterialUtils::colorForMaterial(*mat)
}
} // endof namespace ColorLogicUtils

namespace StorageAuditTable {
namespace RowStyler {

inline void applyStyle(QTableWidget* table, int row, const MaterialMaster* mat, const StorageAuditRow& auditRow) {
    if (!table || !mat) return;

    for (int col = 0; col < table->columnCount(); ++col) {
        QString tip;
        // switch (col) {
        // case ColMaterial:
        //     tip = QString("Anyag: %1\nBarcode: %2").arg(mat->name, mat->barcode);
        //     break;
        // case ColExpected:
        //     tip = QString("Elvárt mennyiség: %1").arg(auditRow.pickingQuantity);
        //     break;
        // case ColActual:
        //     tip = QString("Tényleges mennyiség: %1").arg(auditRow.actualQuantity);
        //     break;
        // case ColMissing:
        //     tip = QString("Hiányzó: %1").arg(auditRow.missingQuantity());
        //     break;
        // case ColStatus:
        //     tip = QString("Státusz: %1").arg(auditRow.status());
        //     break;
        // }


        //TableStyleUtils::ensureStyledItem(table, row, col, bg, fg, Qt::AlignCenter, tip);
    }

    MaterialRowStyler::applyMaterialStyle(table, row, mat, {});
}

} // namespace RowStyler
} // namespace StorageAuditTable
