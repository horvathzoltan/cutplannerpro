// #pragma once

// #include "model/storageaudit/auditstatus_text.h"
// #include "model/storageaudit/storageauditrow.h"
// #include <QColor>


// namespace AuditCellColors {

// inline QColor resolveStatusColor(const StorageAuditRow& row) {
//     AuditStatus status = row.statusType();
//     // return row.isInOptimization
//     //            ? StorageAudit::Status::toColor(status)
//     //            : AuditColors::Info;
//     return StorageAudit::Status::toColor(status);
// }

// // inline QColor resolveActualColor(const StorageAuditRow& row) {
// //     if (!row.isInOptimization || row.pickingQuantity == 0)
// //         return QColor("#f0f0f0"); // semleges szürke – nincs elvárás

// //     int missing = row.context && row.context->group.isGroup()
// //                       ? std::max(0, row.context->totalExpected - row.context->totalActual)
// //                       : std::max(0, row.missingQuantity());

// //     if (missing == 0)
// //         return QColor("#d0f5d0"); // zöld – teljesült

// //     if (missing <= 2)
// //         return QColor("#fff8b0"); // sárga – kis hiány

// //     return QColor("#f8d0d0"); // piros – jelentős hiány
// // }
// }
