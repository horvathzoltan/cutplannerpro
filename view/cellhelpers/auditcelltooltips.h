#pragma once
#include "model/storageaudit/auditcontext.h"
#include "model/material/materialmaster.h"
#include "model/storageaudit/storageauditrow.h"
#include <QString>

namespace AuditCellTooltips {

/**
 * @brief Bővített tooltip: anyag, tároló, sor és csoport szintű adatok.
 *        Hasznos cellaszintű UI‑hoz, ahol a sor és a csoport is látszik.
 */
inline QString toTooltip(const AuditContext* ctx,
                         const MaterialMaster* mat,
                         const StorageAuditRow* row) {
    if (!ctx) return {};

    QStringList parts;

    // 🧩 Anyag adatok
    if (mat) {
        parts << QString("Anyag: %1").arg(mat->name);
        if (!mat->barcode.isEmpty())
            parts << QString("Vonalkód: %1").arg(mat->barcode);
    }

    // 📦 Sor adatok
    if (row) {
        if (!row->storageName.isEmpty())
            parts << QString("Tároló: %1").arg(row->storageName);
        parts << QString("Sor tényleges: %1").arg(row->actualQuantity);
        if (row->isInOptimization)
            parts << "Része az optimalizációnak";
    }

    // 📊 Csoportosított értékek (anyag + rootStorage szinten)
    parts << QString("Elvárt összesen (anyagcsoport): %1").arg(ctx->totalExpected);
    parts << QString("Jelen összesen: %1").arg(ctx->totalActual);
    parts << QString("Hiányzó összesen: %1")
                 .arg(std::max(0, ctx->totalExpected - ctx->totalActual));

    return parts.join("\n");
}

// inline QString formatStatusTooltip(const StorageAuditRow& row, const MaterialMaster* mat) {
//     if (!row.context)
//         return "Nincs audit kontextus.";

//     QString tip = toTooltip(row.context.get(), mat, &row);

//     if (row.context->group.isGroup()) {
//         tip += "\nEz az audit sor egy anyagcsoport tagja — a státusz az egész csoportra vonatkozik.";
//     } else if (!row.isInOptimization) {
//         tip += "\nEz az audit sor nem része az optimalizációnak.";
//     }

//     return tip;
// }

inline QString formatStatusTooltip(const StorageAuditRow& row, const MaterialMaster* mat) {
    QString tip = toTooltip(row.context.get(), mat, &row);

    if (row.context && row.context->group.isGroup()) {
        tip += "\nEz az audit sor egy anyagcsoport tagja — a státusz az egész csoportra vonatkozik.";
    } else if (!row.isInOptimization) {
        tip += "\nEz az audit sor nem része az optimalizációnak.";
    }

    return tip;
}

inline QString formatExpectedTooltip(const StorageAuditRow& row) {
    if (row.sourceType == AuditSourceType::Leftover) {
        if (row.isInOptimization && row.pickingQuantity > 0)
            return QString("Elvárt mennyiség: %1 db\nHulló audit sor – optimalizált").arg(row.pickingQuantity);
        return "Nincs elvárt mennyiség – hulló audit sor";
    }

    if (row.context && row.context->group.isGroup()) {
        if (!row.isInOptimization)
            return "Ez az anyag egy auditcsoport tagja, de nem része az optimalizációnak.";
        return QString("Elvárt mennyiség (anyagcsoport): %1 db").arg(row.context->totalExpected);
    }

    return row.isInOptimization
               ? QString("Elvárt mennyiség: %1 db").arg(row.pickingQuantity)
               : "Nincs elvárt mennyiség – nem része az optimalizációnak.";
}

inline QString formatMissingTooltip(const StorageAuditRow& row) {
    if (!row.isInOptimization || row.pickingQuantity == 0)
        return "Nincs hiányzó mennyiség — nincs elvárt.";

    if (row.context && row.context->group.isGroup()) {
        int missing = std::max(0, row.context->totalExpected - row.context->totalActual);
        return QString("Hiányzó mennyiség (anyagcsoport): %1 db").arg(missing);
    }

    int missing = std::max(0, row.missingQuantity());
    return QString("Hiányzó mennyiség: %1 db").arg(missing);
}
}
