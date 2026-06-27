#pragma once
#include "materials/model/material_family.h"
#include <QString>
#include <QUuid>

struct BomEntry {
    QUuid id;                     // technikai ID
    QUuid productTypeId;          // ProductType
    QUuid productSubtypeId;       // ProductSubtype
    MaterialFamily family;        // MaterialFamily enum
    double quantity;              // mennyiség (db, m, bármi)
};
