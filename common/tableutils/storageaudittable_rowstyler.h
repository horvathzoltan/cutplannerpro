#pragma once

#include "common/grouputils.h"
//#include "common/styleprofiles/auditcolors.h"
//#include "common/tablerowstyler/materialrowstyler.h"
#include "common/tablerowstyler/tablestyleutils.h"
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

    // 🔹 Alapszínek (sor háttér + foreground)
    QColor bg = (auditRow.sourceType == AuditSourceType::Leftover)
                    ? ColorLogicUtils::resolveBaseColor(mat).lighter(120)
                    : GroupUtils::colorForGroup(mat->id);
    QColor fg = (bg.lightness() < 128) ? Qt::white : Qt::black;

    // 🔹 Státusz oszlop kivétele
    QSet<int> excludedCols;
    excludedCols.insert(StorageAuditTableManager::ColStatus);

    for (int col = 0; col < table->columnCount(); ++col) {
        if (excludedCols.contains(col)) continue;
        TableStyleUtils::setCellStyle(table, rowIx, col, bg, fg);
    }


    AuditStatus status = auditRow.statusType();
    QColor statusBg = auditRow.isInOptimization
                          ? StorageAudit::Status::toColor(status)
                          : AuditColors::Info; // halvány szürke, ha nincs optimize alatt

    QColor statusFg = (statusBg.lightness() < 128) ? Qt::white : AuditColors::DefaultFg;
    TableStyleUtils::setCellStyle(table, rowIx, StorageAuditTableManager::ColStatus, statusBg, statusFg);
}



// inline void applyStyle(QTableWidget* table, int rowIx, const MaterialMaster* mat, const StorageAuditRow& auditRow) {
//     if (!table || !mat) return;

//     QColor bg, fg;
//     QSet<int> excludedCols;

//     if (auditRow.sourceType == AuditSourceType::Leftover) {
//         bg = ColorLogicUtils::resolveBaseColor(mat).lighter(120); // halványított csoportszín
//         fg = bg.lightness() < 128 ? Qt::white : Qt::black;

//         excludedCols.insert(StorageAuditTableManager::ColStatus); // státusz külön kezelve
//     } else {
//         bg = GroupUtils::colorForGroup(mat->id);
//         fg = bg.lightness() < 128 ? Qt::white : Qt::black;
//     }

//     for (int col = 0; col < table->columnCount(); ++col) {
//         if (excludedCols.contains(col))
//             continue;

//         TableStyleUtils::setCellStyle(table, rowIx, col, bg, fg);
//     }

//     // 🎯 Státusz cella színezése külön
//     const QString status = auditRow.status();

//     QColor statusColor;
//     if (status == "OK" || status == "Felhasználás alatt, OK")
//         statusColor = AuditColors::ok();
//     else if (status.contains("Hiányzik"))
//         statusColor = AuditColors::missing();
//     else if (status == "Ellenőrzésre vár")
//         statusColor = AuditColors::pending();
//     else
//         statusColor = Qt::lightGray;


//     // 🔄 Csak akkor színezzük, ha az audit sor része az optimalizációnak
//     if (auditRow.isInOptimization) {
//         TableStyleUtils::setCellBackground(table, rowIx, StorageAuditTableManager::ColStatus, statusColor);
//     }

//     // 🔶 Narancsos kiemelés, ha auditált, de nincs tényleges mennyiség
//     if (auditRow.isInOptimization && auditRow.actualQuantity == 0) {
//         TableStyleUtils::setCellBackground(table, rowIx, StorageAuditTableManager::ColStatus, QColor("#ffe0b2")); // narancsos
//     }



//     //TableStyleUtils::setCellBackground(table, rowIx, StorageAuditTableManager::ColStatus, statusColor);
// }


inline void applyTooltips(QTableWidget* table, int rowIx, const MaterialMaster* mat, const StorageAuditRow& auditRow) {
    if (!table || !mat) return;

    for (int col = 0; col < table->columnCount(); ++col) {
        QString tip;

        switch (col) {
        case StorageAuditTableManager::ColMaterial:
            tip = QString("Anyag: %1\nBarcode: %2").arg(mat->name, mat->barcode);
            break;
        case StorageAuditTableManager::ColBarcode:
            tip = QString("Vonalkód: %1").arg(mat->barcode);
            break;
        case StorageAuditTableManager::ColStorage:
            tip = QString("Tároló: %1").arg(auditRow.storageName);
            break;
        case StorageAuditTableManager::ColExpected:
            tip = QString("Elvárt mennyiség: %1").arg(auditRow.pickingQuantity);
            break;
        case StorageAuditTableManager::ColActual:
            tip = auditRow.sourceType == AuditSourceType::Leftover
                      ? "Válaszd ki, hogy az anyag fizikailag jelen van-e.\nEz megerősíti vagy elveti a rendszer állapotát."
                      : QString("Tényleges mennyiség: %1").arg(auditRow.actualQuantity);
            break;
        case StorageAuditTableManager::ColMissing:
            tip = QString("Hiányzó mennyiség: %1").arg(auditRow.missingQuantity());
            break;
        case StorageAuditTableManager::ColStatus:
            tip = QString("Státusz: %1\nEz az audit eredménye az adott anyagra.").arg(auditRow.status());
            break;
        }

        // 🔍 Tooltip alkalmazása itemre vagy widgetre
        if (QTableWidgetItem* item = table->item(rowIx, col)) {
            item->setToolTip(tip);
        } else if (QWidget* widget = table->cellWidget(rowIx, col)) {
            widget->setToolTip(tip);
        }
    }
}


} // namespace RowStyler
} // namespace StorageAuditTable
