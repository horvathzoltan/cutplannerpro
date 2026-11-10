#pragma once

#include "calculation/sizecalcmode.h"
#include "common/surface/surfacetype.h"
#include "worksheetmaterialinfo.h"
#include <QDate>
#include <QString>
#include <QUuid>
#include <common/color/namedcolor.h>
#include "meta/meta.h"


struct WorkSheetOrderInfo
{
    QUuid id = QUuid::createUuid();

    QString externalReference;
    QString ownerName;

    int quantity = 0;

    int fullWidthMm = 0;
    int fullHeightMm = 0;

    QDate dueDate;

    QUuid productTypeId;
    QUuid productSubtypeId;
    SizeCalcMode calcMode =
        SizeCalcMode::Unknown;

    int requestCount = 0;

    int completedCount = 0;
    int uncompletedCount = 0;
    int undefinedCount = 0;

    NamedColor requiredColor;
    SurfaceType requiredSurface = SurfaceType::Unknown;
    QVector<WorkSheetMaterialInfo> materials;

    bool requiresAttention() const
    {
        return
            undefinedCount > 0 ||
            uncompletedCount > 0;
    }


private:
    static MetaData<WorkSheetOrderInfo> _meta;
public:
    static MetaData<WorkSheetOrderInfo>& Meta();
    static void MetaInit();
};