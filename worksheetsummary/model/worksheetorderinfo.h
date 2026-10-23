#pragma once

#include "common/surface/surfacetype.h"
#include <QDate>
#include <QString>
#include <QUuid>

#include <common/color/namedcolor.h>


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

    //QString requiredColorCode;
    NamedColor requiredColor;
    SurfaceType requiredSurface = SurfaceType::Unknown;
    QUuid materialId;

    bool requiresAttention() const
    {
        return
            undefinedCount > 0 ||
            uncompletedCount > 0;
    }
};