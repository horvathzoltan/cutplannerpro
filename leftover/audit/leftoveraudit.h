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
};
