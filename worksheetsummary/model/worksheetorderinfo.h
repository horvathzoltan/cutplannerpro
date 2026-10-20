#pragma once

#include <QDate>
#include <QString>
#include <QUuid>


struct WorkSheetOrderInfo
{
    QString externalReference;

    QString ownerName;

    QDate dueDate;

    QUuid productTypeId;
    QUuid productSubtypeId;

    int requestCount = 0;

    int completedCount = 0;
    int uncompletedCount = 0;
    int undefinedCount = 0;

    QString requiredColorCode;

    bool requiresAttention() const
    {
        return
            undefinedCount > 0 ||
            uncompletedCount > 0;
    }
};