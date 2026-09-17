#pragma once
#include "materials/model/material_family.h"
#include <QString>
#include <QUuid>

struct MaterialRole {
    QUuid productTypeId;
    QUuid productSubtypeId;
    MaterialFamily family;
    //QString barcodePrefix;

    QUuid groupId;          // gyártási szerepkör-csoport (MaterialRoleGroup)
    QUuid storageGroupId;   // tárolási szerepkör-csoport (StorageRoleGroup)

    bool operator<(const MaterialRole& other) const {
        return std::tie(productTypeId,
                        productSubtypeId,
                        family,
                        groupId,
                        storageGroupId)
               < std::tie(other.productTypeId,
                          other.productSubtypeId,
                          other.family,
                          other.groupId,
                          other.storageGroupId);
    }
};
