#pragma once
#include "materials/model/material_family.h"
#include <QString>
#include <QUuid>

struct MaterialRole {
    QUuid productTypeId;
    QUuid productSubtypeId;
    MaterialFamily family;
    QString barcodePrefix;
};
