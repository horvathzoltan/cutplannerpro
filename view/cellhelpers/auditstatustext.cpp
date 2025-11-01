#include "auditstatustext.h"

namespace AuditStatusText {

QString toText(const AuditStatus& s) {
    return s.toDecoratedText(); // meglévő AuditStatus metódus
}

QString suffixForRow(const StorageAuditRow& row) {
    if (row.sourceType == AuditSourceType::Leftover) {
        if (!row.isAudited())
            return "Hulló nem auditált";
        return row.isFulfilled() ? "Hulló van" : "Hulló nincs";
    }

    if (row.isRowModified)
        return row.isFulfilled() ? "Módosítva" : "Módosítva – nincs készlet";

    if (row.isRowAuditChecked && !row.isFulfilled())
        return "Nincs készlet";

    return {};
}

QString decorated(const StorageAuditRow& row) {
    QString base = toText(row.statusType());
    QString suffix = suffixForRow(row);
    return suffix.isEmpty() ? base : base + " — " + suffix;
}

} // namespace AuditStatusText
