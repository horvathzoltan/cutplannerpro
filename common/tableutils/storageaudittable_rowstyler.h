#pragma once

#include "common/grouputils.h"
#include "common/tablerowstyler/materialrowstyler.h"
#include "model/material/materialmaster.h"
#include "model/storageaudit/storageauditrow.h"
#include <QTableWidget>
#include <view/managers/storageaudittable_manager.h>



namespace ColorLogicUtils{
inline QColor resolveBaseColor(const MaterialMaster* mat) {
    if (!mat) return QColor(Qt::lightGray);
    return GroupUtils::colorForGroup(mat->id); // vagy MaterialUtils::colorForMaterial(*mat)
}
} // endof namespace ColorLogicUtils

namespace StorageAuditTable {
namespace RowStyler {

inline void applyStyle(QTableWidget* table, int rowIx, const MaterialMaster* mat, const StorageAuditRow& auditRow) {
    if (!table || !mat) return;

    QSet<int> excludedCols;
    if (auditRow.sourceType == AuditSourceType::Leftover) {
        excludedCols.insert(StorageAuditTableManager::ColActual);   // checkbox cella
        excludedCols.insert(StorageAuditTableManager::ColMissing);  // nem releváns
        excludedCols.insert(StorageAuditTableManager::ColStatus);   // külön színkód
    }

    // Leftover sorok: halványkék háttér
    if (auditRow.sourceType == AuditSourceType::Leftover) {
        QColor bg = ColorLogicUtils::resolveBaseColor(mat); // dinamikus szín
        QColor fg = bg.lightness() < 128 ? Qt::white : Qt::black;

        for (int col = 0; col < table->columnCount(); ++col) {
            if (excludedCols.contains(col))
                continue;

            QTableWidgetItem* item = table->item(rowIx, col);
            if (!item) {
                item = new QTableWidgetItem();
                table->setItem(rowIx, col, item);
            }

            item->setBackground(bg);
            item->setForeground(fg);
            item->setTextAlignment(Qt::AlignCenter);
        }
    } else {
        // Stock audit sorok: anyag színkód
        MaterialRowStyler::applyMaterialStyle(table, rowIx, mat, excludedCols);
    }

    // Státusz cella színezése külön
    auto* statusItem = table->item(rowIx, StorageAuditTableManager::ColStatus);
    if (statusItem) {
        const QString status = auditRow.status();
        if (status == "OK")
            statusItem->setBackground(Qt::green);
        else if (status == "Hiányzik")
            statusItem->setBackground(Qt::red);
        else
            statusItem->setBackground(Qt::lightGray);
    }
}

inline void applyTooltips(QTableWidget* table, int rowIx, const MaterialMaster* mat, const StorageAuditRow& auditRow) {
    if (!table || !mat) return;

    for (int col = 0; col < table->columnCount(); ++col) {
        QTableWidgetItem* item = table->item(rowIx, col);
        if (!item) continue;

        QString tip;
        switch (col) {
        case StorageAuditTableManager::ColMaterial:
            tip = QString("Anyag: %1\nBarcode: %2").arg(mat->name, mat->barcode);
            break;
        case StorageAuditTableManager::ColStorage:
            tip = QString("Tároló: %1").arg(auditRow.storageName());
            break;
        case StorageAuditTableManager::ColExpected:
            tip = QString("Elvárt mennyiség: %1").arg(auditRow.pickingQuantity);
            break;
        case StorageAuditTableManager::ColActual:
            tip = auditRow.sourceType == AuditSourceType::Leftover
                      ? "Pipáld, ha fizikailag ott van"
                      : QString("Tényleges mennyiség: %1").arg(auditRow.actualQuantity);
            break;
        case StorageAuditTableManager::ColMissing:
            tip = QString("Hiányzó: %1").arg(auditRow.missingQuantity());
            break;
        case StorageAuditTableManager::ColStatus:
            tip = QString("Státusz: %1").arg(auditRow.status());
            break;
        }

        item->setToolTip(tip);
    }
}

// inline void applyStyle(QTableWidget* table, int row, const MaterialMaster* mat, const StorageAuditRow& auditRow) {
//     if (!table || !mat) return;

//     // for (int col = 0; col < table->columnCount(); ++col) {
//     //     QString tip;
//     //     // switch (col) {
//     //     // case ColMaterial:
//     //     //     tip = QString("Anyag: %1\nBarcode: %2").arg(mat->name, mat->barcode);
//     //     //     break;
//     //     // case ColExpected:
//     //     //     tip = QString("Elvárt mennyiség: %1").arg(auditRow.pickingQuantity);
//     //     //     break;
//     //     // case ColActual:
//     //     //     tip = QString("Tényleges mennyiség: %1").arg(auditRow.actualQuantity);
//     //     //     break;
//     //     // case ColMissing:
//     //     //     tip = QString("Hiányzó: %1").arg(auditRow.missingQuantity());
//     //     //     break;
//     //     // case ColStatus:
//     //     //     tip = QString("Státusz: %1").arg(auditRow.status());
//     //     //     break;
//     //     // }


//     //     //TableStyleUtils::ensureStyledItem(table, row, col, bg, fg, Qt::AlignCenter, tip);
//     // }

//     MaterialRowStyler::applyMaterialStyle(table, row, mat, {});
// }

} // namespace RowStyler
} // namespace StorageAuditTable
