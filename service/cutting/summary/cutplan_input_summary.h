#pragma once
#include <QString>
#include <QUuid>
#include <QVector>

struct CutPlanInputSummary {

    struct Item {
        QString externalRef;
        int length_mm = 0;
        int quantity = 0;
    };

    struct MaterialBlock {
        QUuid materialId;
        QString materialName;
        QString materialBarcode;
        QString materialGroupName;
        QVector<Item> items;
        int totalItems = 0;
        int totalQuantity = 0;
    };

    struct OwnerBlock {
        QString ownerName;
        QVector<MaterialBlock> materials;
    };

    QVector<OwnerBlock> owners;

    QString toText() const;
};
