#include "leftoveraudit.h"
#include "leftover/registry/leftoverstockregistry.h"
#include <QDateTime>

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

