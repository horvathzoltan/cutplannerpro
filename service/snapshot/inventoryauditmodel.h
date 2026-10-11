#pragma once

#include "requestsnapshotbuilder.h"

#include <QString>
#include <QVector>
#include <QUuid>

enum class InventoryCoverageSource
{
    Undefined,

    OriginalStock,
    AlternativeMaterial,
    ComponentAssembly,

    Mixed,

    Missing
};

struct InventoryAuditComponent
{
    QUuid materialId;

    QString barcode;

    QString materialName;

    int requiredPerBundle = 0;

    int stock = 0;

    int assemblableCount = 0;
};

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

    InventoryCoverageSource coverageSource =
        InventoryCoverageSource::Undefined;

    int originalStock = 0;

    int alternativeStock = 0;

    int componentAssemblyStock = 0;

    int availableStock = 0;

    QVector<InventoryAuditComponent>
        assemblyComponents;

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

    // Coverage
    int requiredDemand = 0;

    int originalAvailable = 0;
    int alternativeAvailable = 0;

    int totalAvailable = 0;

    GroupCoverage coverage;

    // Audit info
    int originalEstimate = 0;
    int alternativeEstimate = 0;
};


struct InventoryAuditModel
{
    QVector<InventoryAuditRow> rows;
    QVector<InventoryAuditGroupSummary> groups;
};