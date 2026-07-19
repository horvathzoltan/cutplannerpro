#pragma once
#include "materials/model/material_family.h"
#include <QString>
#include <QUuid>

struct MaterialRole {
    QUuid productTypeId;
    QUuid productSubtypeId;
    MaterialFamily family;
    QString barcodePrefix;

    bool operator<(const MaterialRole& other) const {
        return std::tie(productTypeId, productSubtypeId, family, barcodePrefix)
        < std::tie(other.productTypeId, other.productSubtypeId, other.family, other.barcodePrefix);
    }

};
