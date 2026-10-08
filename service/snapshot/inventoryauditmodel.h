#pragma once

#include "requestsnapshotbuilder.h"

#include <QString>
#include <QVector>
#include <QUuid>

struct InventoryAuditRow
{
    QUuid materialId;

    QString barcode;
    QString materialName;

    QString materialGroup;

    bool isExpanded = false;
    QString sourceBarcode;

    int requestPieces = 0;

    qint64 requestedLengthMm = 0;

    QVector<int> lengths;

    int estimatedStrands = 0;

    int stockStrands = 0;

    int snapshotStrands = 0;

    bool hasRequest = false;

    QUuid originMaterialId;

    RequestSnapshotBuilder::MaterialDemandOrigin
        origin =
        RequestSnapshotBuilder::MaterialDemandOrigin::Request;

};

enum class GroupCoverage
{
    Original,
    Partial,
    Substitute,
    Shortage
};

struct InventoryAuditGroupSummary
{
    QString groupKey;
    QString groupName;

    int originalGreedyStrands = 0;
    int expandedGreedyStrands = 0;

    int totalGreedyStrands = 0;

    int totalStockStrands = 0;
    int totalSnapshotStrands = 0;

    GroupCoverage coverage;
};


struct InventoryAuditModel
{
    QVector<InventoryAuditRow> rows;
    QVector<InventoryAuditGroupSummary> groups;
};