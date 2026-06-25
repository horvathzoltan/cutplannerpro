#include "naphalo_due_rules.h"

NaphaloDueAuditResult NaphaloDueRules::check(const QVector<Cutting::Plan::Request>& list)
{
    NaphaloDueAuditResult r;

    if (list.isEmpty())
        return r;

    // 🔹 Canonical due date
    QDate expected = list.first().dueDate;
    r.expectedDue = expected;

    // 🔹 Determinisztikus ellenőrzés
    for (int i = 1; i < list.size(); ++i) {
        if (list[i].dueDate != expected) {
            r.hasError = true;
            r.messages << QString("❌ Határidő eltérés: %1 ↔ %2")
                              .arg(list[i].dueDate.toString("yyyy-MM-dd"))
                              .arg(expected.toString("yyyy-MM-dd"));
        }
    }

    return r;
}
