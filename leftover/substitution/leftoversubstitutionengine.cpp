#include "leftoversubstitutionengine.h"
#include <leftover/audit/leftoveraudit.h>

LeftoverSubstitutionResult LeftoverSubstitutionEngine::run(
    const QVector<LeftoverStockEntry>& targets,
    int daysThreshold,
    int tolerance_mm)
{
    LeftoverSubstitutionResult result;
    result.targets = targets;

    // 1️⃣ Friss leftover pool
    QVector<LeftoverStockEntry> freshPool =
        LeftoverAudit::collectFresh(daysThreshold);

    // targeteket kizárjuk
    {
        QSet<QUuid> tIds;
        for (const auto& t : targets)
            tIds.insert(t.entryId);

        QVector<LeftoverStockEntry> filtered;
        for (const auto& e : freshPool)
            if (!tIds.contains(e.entryId))
                filtered.append(e);

        freshPool = filtered;
    }

    // 2️⃣ Matching
    QHash<QUuid, QUuid> mapping =
        LeftoverAudit::matchSubstitutes(targets, freshPool, tolerance_mm);

    result.mapping = mapping;

    // 3️⃣ Felhasznált helyettesítők
    QHash<QUuid, LeftoverStockEntry> byId;
    for (const auto& e : freshPool)
        byId[e.entryId] = e;

    for (auto it = mapping.begin(); it != mapping.end(); ++it) {
        QUuid subId = it.value();
        if (byId.contains(subId))
            result.substitutesUsed.append(byId[subId]);
    }

    // 4️⃣ Akikhez nem találtunk helyettesítőt
    for (const auto& t : targets) {
        if (!mapping.contains(t.entryId))
            result.substitutesMissing.append(t);
    }

    return result;
}
