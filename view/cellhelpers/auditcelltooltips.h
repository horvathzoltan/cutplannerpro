#pragma once
#include "model/storageaudit/storageauditrow.h"
#include "model/material/materialmaster.h"
#include "view/cellhelpers/auditcelltext.h"
#include <QString>

namespace AuditCellTooltips {

// auditcelltooltips.h

inline QString forStatus(const StorageAuditRow& row, const MaterialMaster* mat) {
    QStringList lines;

    // 🔑 Emberi azonosítók
   // if (!row.rodId.isEmpty())
   //     lines << QString("RodId: %1").arg(row.rodId);
    lines << QString("Barcode: %1").arg(row.barcode.isEmpty() ? "—" : row.barcode);

    // 📦 Anyag és tároló
    if (mat)
        lines << QString("Anyag: %1").arg(mat->name);
    if (!row.storageName.isEmpty())
        lines << QString("Tároló: %1").arg(row.storageName);

    // 📊 Context adatok
    if (row.hasContext()) {
        lines << QString("Auditcsoport: %1 (%2 tag)")
        .arg(row.groupKey())
            .arg(row.groupSize());
        lines << QString("Elvárt összesen: %1").arg(row.totalExpected());
        lines << QString("Tényleges összesen: %1").arg(row.totalActual());
        lines << QString("Hiányzó összesen: %1").arg(row.missingQuantity());
    }

    // 📋 Sor szintű adatok
    lines << QString("Elvárt (sor): %1").arg(row.totalExpected());
    lines << QString("Tényleges (sor): %1").arg(row.actualQuantity);
    lines << QString("Hiányzó (sor): %1").arg(row.missingQuantity());

    // 🟢 Státusz (suffixekkel együtt)
    lines << QString("Státusz: %1").arg(row.statusText());

    if (!row.isInOptimization)
        lines << "⚠️ Nem része az optimalizációnak";

    return lines.join("\n");
}




inline QString forExpected(const StorageAuditRow& row, const QString& groupLabel = "") {
    return AuditCellText::forExpected(row, groupLabel);
}

inline QString forMissing(const StorageAuditRow& row) {
    return AuditCellText::forMissing(row);
}


} // namespace AuditCellTooltips

// #pragma once
// #include "model/storageaudit/auditcontext.h"
// #include "model/material/materialmaster.h"
// #include "model/storageaudit/storageauditrow.h"
// #include <QString>

// namespace AuditCellTooltips {

// /**
//  * @brief Bővített tooltip: anyag, tároló, sor és csoport szintű adatok.
//  *        Hasznos cellaszintű UI‑hoz, ahol a sor és a csoport is látszik.
//  */
// inline QString toTooltip(const AuditContext* ctx,
//                          const MaterialMaster* mat,
//                          const StorageAuditRow* row) {
//     if (!ctx) return {};

//     QStringList parts;

//     // 🧩 Anyag adatok
//     if (mat) {
//         parts << QString("Anyag: %1").arg(mat->name);
//         if (!mat->barcode.isEmpty())
//             parts << QString("Vonalkód: %1").arg(mat->barcode);
//     }

//     // 📦 Sor adatok
//     if (row) {
//         if (!row->storageName.isEmpty())
//             parts << QString("Tároló: %1").arg(row->storageName);
//         parts << QString("Sor tényleges: %1").arg(row->actualQuantity);
//         if (row->isInOptimization)
//             parts << "Része az optimalizációnak";
//     }

//     // 📊 Csoportosított értékek (anyag + rootStorage szinten)
//     parts << QString("Elvárt összesen (anyagcsoport): %1").arg(ctx->totalExpected);
//     parts << QString("Jelen összesen: %1").arg(ctx->totalActual);
//     parts << QString("Hiányzó összesen: %1")
//                  .arg(std::max(0, ctx->totalExpected - ctx->totalActual));

//     return parts.join("\n");
// }

// inline QString formatStatusTooltip(const StorageAuditRow& row, const MaterialMaster* mat) {
//     QString tip = toTooltip(row.context.get(), mat, &row);

//     if (row.context && row.context->group.isGroup()) {
//         tip += "\nEz az audit sor egy anyagcsoport tagja — a státusz az egész csoportra vonatkozik.";
//     } else if (!row.isInOptimization) {
//         tip += "\nEz az audit sor nem része az optimalizációnak.";
//     }

//     return tip;
// }

// inline QString formatExpectedTooltip(const StorageAuditRow& row) {
//     if (row.sourceType == AuditSourceType::Leftover) {
//         if (row.isInOptimization && row.pickingQuantity > 0)
//             return QString("Elvárt mennyiség: %1 db\nHulló audit sor – optimalizált").arg(row.pickingQuantity);
//         return "Nincs elvárt mennyiség – hulló audit sor";
//     }

//     if (row.context && row.context->group.isGroup()) {
//         if (!row.isInOptimization)
//             return "Ez az anyag egy auditcsoport tagja, de nem része az optimalizációnak.";
//         return QString("Elvárt mennyiség (anyagcsoport): %1 db").arg(row.context->totalExpected);
//     }

//     return row.isInOptimization
//                ? QString("Elvárt mennyiség: %1 db").arg(row.pickingQuantity)
//                : "Nincs elvárt mennyiség – nem része az optimalizációnak.";
// }

// inline QString formatMissingTooltip(const StorageAuditRow& row) {
//     if (!row.isInOptimization || row.pickingQuantity == 0)
//         return "Nincs hiányzó mennyiség — nincs elvárt.";

//     if (row.context && row.context->group.isGroup()) {
//         int missing = std::max(0, row.context->totalExpected - row.context->totalActual);
//         return QString("Hiányzó mennyiség (anyagcsoport): %1 db").arg(missing);
//     }

//     int missing = std::max(0, row.missingQuantity());
//     return QString("Hiányzó mennyiség: %1 db").arg(missing);
// }

// QString tooltipTextForRow(const StorageAuditRow& row)
// {
//     QStringList lines;

//     // 🔹 Auditcsoport tagság
//     if (row.context) {
//         lines << QString("Auditcsoport: %1 (%2 tag)")
//         .arg(row.context->group.groupKey())
//             .arg(row.context->group.size());
//     } else {
//         lines << "Auditcsoport: nincs hozzárendelve";
//     }

//     // ✂️ Optimalizációs tagság
//     if (row.isInOptimization) {
//         lines << "Optimalizáció: része a vágási tervnek";
//     } else {
//         lines << "Optimalizáció: nem része a vágási tervnek";
//     }

//     // 📦 Elvárt és tényleges mennyiség
//     lines << QString("Elvárt: %1 db").arg(row.pickingQuantity);
//     lines << QString("Tényleges: %1 db").arg(row.actualQuantity);

//     // 📉 Hiányzó mennyiség (soronként vagy csoportosan)
//     int missing = row.missingQuantity();
//     lines << QString("Hiányzó: %1 db").arg(missing);

//     // 🟢 Státusz szöveg (színes, suffixes)
//     lines << QString("Státusz: %1").arg(row.statusText());

//     return lines.join("\n");
// }

// } // namespace AuditCellTooltips


