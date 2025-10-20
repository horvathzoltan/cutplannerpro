// #pragma once

// #include "model/storageaudit/storageauditrow.h"
// #include <QString>

// namespace TableUtils::AuditCells {

// /**
//  * @brief Elvárt mennyiség szöveges formázása audit sorhoz.
//  *        Ha a sor csoport tagja, akkor a csoport összesített értékét jeleníti meg.
//  */
// // inline QString expectedText(const StorageAuditRow& row) {
// //     if (row.context && row.context->group.isGroup()) {
// //         return QString("%1 db (anyagcsoport)").arg(row.context->totalExpected);
// //     }
// //     return row.pickingQuantity > 0
// //                ? QString::number(row.pickingQuantity)
// //                : QString();
// // }

// /**
//  * @brief Hiányzó mennyiség szöveges formázása audit sorhoz.
//  *        Ha a sor csoport tagja, akkor a csoport szintű hiányt jeleníti meg.
// //  */
// // inline QString missingText(const StorageAuditRow& row) {
// //     if (row.context && row.context->group.isGroup()) {
// //         int missing = row.context->totalExpected - row.context->totalActual;
// //         return QString("%1 db (anyagcsoport)").arg(missing);
// //     }
// //     return row.pickingQuantity > 0
// //                ? QString::number(row.missingQuantity())
// //                : QString();
// // }

// /**
//  * @brief Audit státusz szöveges formázása cellához.
//  *        A StorageAuditRow::statusText() logikáját használja.
//  */
// // inline QString statusText(const StorageAuditRow& row) {
// //     return row.statusText();
// // }

// // inline QString actualText(const StorageAuditRow& row) {
// //     return QString::number(row.actualQuantity);
// // }

// // inline AuditStatus statusType(const StorageAuditRow& row) {
// //     return row.statusType();
// // }


// } // namespace TableUtils::AuditCells
