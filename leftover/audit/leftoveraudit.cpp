#include "leftoveraudit.h"
#include "leftover/registry/leftoverstockregistry.h"
#include <QDateTime>
#include "common/logger.h"

QVector<LeftoverStockEntry> LeftoverAudit::collectExpired(int daysThreshold)
{
    QVector<LeftoverStockEntry> all = LeftoverStockRegistry::instance().readAll();
    QVector<LeftoverStockEntry> expired;

    const QDateTime now = QDateTime::currentDateTime();

    for (const auto& e : all) {
        if (e.lastSeenAt.daysTo(now) > daysThreshold)
            expired.append(e);
    }
    return expired;
}

QVector<LeftoverStockEntry> LeftoverAudit::collectStorageAudit(
    const QUuid& storageId,
    const QVector<QUuid>& materialIds,
    int daysThreshold)
{
    QVector<LeftoverStockEntry> all = LeftoverStockRegistry::instance().readAll();
    QVector<LeftoverStockEntry> result;

    const QDateTime now = QDateTime::currentDateTime();

    for (const auto& e : all) {
        if (e.storageId != storageId)
            continue;

        if (!materialIds.contains(e.materialId))
            continue;

        if (e.lastSeenAt.daysTo(now) > daysThreshold)
            result.append(e);
    }
    return result;
}

QVector<LeftoverStockEntry> LeftoverAudit::collectMissing()
{
    QVector<LeftoverStockEntry> all = LeftoverStockRegistry::instance().readAll();
    QVector<LeftoverStockEntry> missing;

    for (const auto& e : all) {
        if (e.notFoundCount > 0)
            missing.append(e);
    }
    return missing;
}

QVector<LeftoverStockEntry> LeftoverAudit::collectProfileAudit(
    const QVector<QUuid>& materialIds,
    int daysThreshold)
{
    QVector<LeftoverStockEntry> all = LeftoverStockRegistry::instance().readAll();
    QVector<LeftoverStockEntry> result;

    const QDateTime now = QDateTime::currentDateTime();

    for (const auto& e : all) {
        if (!materialIds.contains(e.materialId))
            continue;

        if (e.lastSeenAt.daysTo(now) > daysThreshold)
            result.append(e);
    }
    return result;
}

QVector<LeftoverStockEntry> LeftoverAudit::collectUsedLeftovers(
    const QVector<QUuid>& usedIds)
{
    QVector<LeftoverStockEntry> all =
        LeftoverStockRegistry::instance().readAll();

    QVector<LeftoverStockEntry> result;

    for (const auto& e : all) {
        if (usedIds.contains(e.entryId))
            result.append(e);
    }

    return result;
}

QVector<LeftoverStockEntry> LeftoverAudit::collectFresh(int daysThreshold)
{
    QVector<LeftoverStockEntry> all = LeftoverStockRegistry::instance().readAll();
    QVector<LeftoverStockEntry> fresh;

    const QDateTime now = QDateTime::currentDateTime();

    for (const auto& e : all) {
        if (e.notFoundCount > 0)
            continue;

        if (e.lastSeenAt.daysTo(now) > daysThreshold)
            continue;

        fresh.append(e);
    }
    return fresh;
}


QVector<LeftoverStockEntry> LeftoverAudit::collectSubstitutesFor(
    const LeftoverStockEntry& target,
    const QVector<LeftoverStockEntry>& pool,
    int tolerance_mm)
{

    auto * mat = MaterialRegistry::instance().findById(target.materialId);

    QString matName = mat?mat->toReportLabel():"?";

    zInfo(QString("🔍 Helyettesítő keresés: target=%1 len=%2 mat=%3")
              .arg(target.barcode)
              .arg(target.availableLength_mm)
              .arg(matName));


    QVector<LeftoverStockEntry> result;

    int i = 0;
    int j = 0;

    for (const auto& e : pool) {

        if (e.materialId != target.materialId)
            continue;

        if (e.availableLength_mm < target.availableLength_mm)
            continue;

        if (e.availableLength_mm > target.availableLength_mm){
            i++;
        }

        if (e.availableLength_mm > target.availableLength_mm + tolerance_mm)
            continue;

        j++;
        zInfo(QString("  ➕ Jelölt: %1 len=%2 lastSeen=%3 missing=%4")
                  .arg(e.barcode)
                  .arg(e.availableLength_mm)
                  .arg(e.lastSeenAt.toString())
                  .arg(e.notFoundCount));

        result.append(e);
    }

    QString a = QString("nagyobb: %1 db, ebből megfelelő: %2 db" ).arg(i).arg(j);

    return result;
}


QHash<QUuid, QUuid> LeftoverAudit::matchSubstitutes(
    const QVector<LeftoverStockEntry>& targets,
    const QVector<LeftoverStockEntry>& pool,
    int tolerance_mm)
{
    zInfo(QString("🔧 Matching indul: targets=%1 pool=%2")
              .arg(targets.size())
              .arg(pool.size()));


    QHash<QUuid, QUuid> mapping;

    QVector<LeftoverStockEntry> available = pool;

    QVector<LeftoverStockEntry> sortedTargets = targets;
    std::sort(sortedTargets.begin(), sortedTargets.end(),
              [](const LeftoverStockEntry& a, const LeftoverStockEntry& b){
                  return a.availableLength_mm > b.availableLength_mm;
              });

    for (const auto& t : sortedTargets) {

        QVector<LeftoverStockEntry> candidates =
            collectSubstitutesFor(t, available, tolerance_mm);

        if (candidates.isEmpty()) {
            zInfo(QString("    ❌ Nincs helyettesítő: %1").arg(t.barcode));
            continue;
        }


        std::sort(candidates.begin(), candidates.end(),
                  [](const LeftoverStockEntry& a, const LeftoverStockEntry& b){
                      return a.availableLength_mm < b.availableLength_mm;
                  });

        LeftoverStockEntry chosen = candidates.first();
        mapping[t.entryId] = chosen.entryId;

        zInfo(QString("    ✅ Helyettesítő: %1 → %2")
                  .arg(t.barcode)
                  .arg(chosen.barcode));

        for (int i = 0; i < available.size(); ++i) {
            if (available[i].entryId == chosen.entryId) {
                available.removeAt(i);
                break;
            }
        }
    }

    return mapping;
}

QVector<LeftoverStockEntry> LeftoverAudit::collectSubstituteCandidates(
    const LeftoverStockEntry& target,
    const QVector<LeftoverStockEntry>& pool,
    int tolerance_mm,
    int maxCandidates)
{
    QVector<LeftoverStockEntry> result;

    for (const auto& e : pool) {

        if (e.entryId == target.entryId)
            continue;

        if (e.materialId != target.materialId)
            continue;

        if (e.availableLength_mm < target.availableLength_mm)
            continue;

        if (e.availableLength_mm > target.availableLength_mm + tolerance_mm)
            continue;

        if (e.notFoundCount > 0)
            continue;

        result.append(e);
    }

    std::sort(result.begin(), result.end(),
              [](const LeftoverStockEntry& a, const LeftoverStockEntry& b){
                  return a.availableLength_mm < b.availableLength_mm;
              });

    if (result.size() > maxCandidates)
        result = result.mid(0, maxCandidates);

    return result;
}

QHash<QUuid, QUuid> LeftoverAudit::matchAfterAudit(
    const QVector<LeftoverStockEntry>& targets,
    const QHash<QUuid, QVector<LeftoverStockEntry>>& candidateSets,
    const QSet<QUuid>& presentIds)
{
    QHash<QUuid, QUuid> mapping;

    for (const auto& t : targets) {

        if (presentIds.contains(t.entryId)) {
            mapping[t.entryId] = t.entryId;
            continue;
        }

        const auto& candidates = candidateSets.value(t.entryId);

        for (const auto& c : candidates) {
            if (presentIds.contains(c.entryId)) {
                mapping[t.entryId] = c.entryId;
                break;
            }
        }
    }

    return mapping;
}

