#pragma once
#include "auditcontext.h"
#include "model/material/materialmaster.h"
#include "model/storageaudit/storageauditrow.h"
#include <QString>

namespace StorageAudit {
namespace Context {

/**
 * @brief Egyszerű tooltip: csak összesített értékek a contextből.
 *        Hasznos aggregált audit nézetekhez.
 */
inline QString toTooltip(const AuditContext* ctx) {
    if (!ctx) return {};
    return QString("Elvárt összesen (anyagcsoport): %1\nJelen összesen: %2")
        .arg(ctx->group.totalExpected)
        .arg(ctx->group.totalActual);
}

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
    parts << QString("Elvárt összesen (anyagcsoport): %1").arg(ctx->group.totalExpected);
    parts << QString("Jelen összesen: %1").arg(ctx->group.totalActual);
    parts << QString("Hiányzó összesen: %1")
                 .arg(std::max(0, ctx->group.totalExpected - ctx->group.totalActual));

    return parts.join("\n");
}

} // namespace Context
} // namespace StorageAudit
