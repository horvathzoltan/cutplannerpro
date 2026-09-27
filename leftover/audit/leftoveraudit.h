#pragma once
#include <QVector>
#include "leftover/model/leftoverstockentry.h"

class LeftoverAudit {
public:
    static QVector<LeftoverStockEntry> collectExpired(int daysThreshold);
    static QVector<LeftoverStockEntry> collectStorageAudit(
        const QUuid& storageId,
        const QVector<QUuid>& materialIds,
        int daysThreshold);

    static QVector<LeftoverStockEntry> collectMissing();
    static QVector<LeftoverStockEntry> collectProfileAudit(
        const QVector<QUuid>& materialIds,
        int daysThreshold);

    static QVector<LeftoverStockEntry> collectUsedLeftovers(
        const QVector<QUuid>& usedIds);

    static QVector<LeftoverStockEntry> collectFresh(int daysThreshold);
    static QVector<LeftoverStockEntry> collectSubstitutesFor(
        const LeftoverStockEntry& target,
        const QVector<LeftoverStockEntry>& pool,
        int tolerance_mm);

    static QHash<QUuid, QUuid> matchSubstitutes(
        const QVector<LeftoverStockEntry>& targets,
        const QVector<LeftoverStockEntry>& pool,
        int tolerance_mm);

    static QVector<LeftoverStockEntry> collectSubstituteCandidates(const LeftoverStockEntry &target, const QVector<LeftoverStockEntry> &pool, int tolerance_mm, int maxCandidates);
    static QHash<QUuid, QUuid> matchAfterAudit(const QVector<LeftoverStockEntry> &targets, const QHash<QUuid, QVector<LeftoverStockEntry> > &candidateSets, const QSet<QUuid> &presentIds);
};
