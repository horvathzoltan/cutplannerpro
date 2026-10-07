#pragma once

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

    bool hasShortage = false;
};


struct InventoryAuditModel
{
    QVector<InventoryAuditRow> rows;
    QVector<InventoryAuditGroupSummary> groups;
};