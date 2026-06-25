#include "naphalo_owner_rules.h"

NaphaloOwnerAuditResult NaphaloOwnerRules::check(const QVector<Cutting::Plan::Request>& list)
{
    NaphaloOwnerAuditResult r;

    if (list.isEmpty())
        return r;

    // 🔹 Canonical owner
    QString expected = list.first().ownerName.trimmed();
    if (expected.isEmpty())
        expected = "<ismeretlen>";

    r.expectedOwner = expected;

    // 🔹 Determinisztikus ellenőrzés
    for (int i = 1; i < list.size(); ++i) {
        QString o = list[i].ownerName.trimmed();
        if (o.isEmpty())
            o = "<ismeretlen>";

        if (o != expected) {
            r.hasError = true;
            r.messages << QString("❌ Megrendelő eltérés: %1 ↔ %2")
                              .arg(o)
                              .arg(expected);
        }
    }

    return r;
}
