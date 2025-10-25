#include "leftoverstockentry.h"
#include "registries/materialregistry.h"
#include "model/material/materialgroup_utils.h"

#include <model/registries/storageregistry.h>

const MaterialMaster* LeftoverStockEntry::master() const {
    if (materialId.isNull())
        return nullptr;

    return MaterialRegistry::instance().findById(materialId);
}

QString LeftoverStockEntry::materialName() const {
    const auto* m = master();
    return m ? m->name : "(?)";
}

QString LeftoverStockEntry::materialBarcode() const {
    const auto* m = master();
    return m ? m->barcode : "(?)";
}

QString LeftoverStockEntry::reusableBarcode() const {
    return barcode.isEmpty()? "(?)" : barcode;
}

MaterialType LeftoverStockEntry::materialType() const {
    const auto* m = master();
    return m ? m->type : MaterialType(MaterialType::Type::Unknown);
}

QString LeftoverStockEntry::materialGroupName() const {
    return GroupUtils::groupName(materialId);
}

QColor LeftoverStockEntry::materialGroupColor() const {
    return GroupUtils::colorForGroup(materialId);
}

bool LeftoverStockEntry::operator==(const LeftoverStockEntry& other) const {
    return materialId == other.materialId &&
           availableLength_mm == other.availableLength_mm;
}

QString LeftoverStockEntry::sourceAsString() const
{
    if (source == Cutting::Result::LeftoverSource::Manual)
        return "Manuális";
    if (source == Cutting::Result::LeftoverSource::Optimization)
        return optimizationId.has_value()
                   ? QString("Op:%1").arg(*optimizationId)
                   : "Op:?";
    return "Ismeretlen";
}

QString LeftoverStockEntry::storageName() const {
    const StorageEntry *storage =
        StorageRegistry::instance().findById(storageId);
    return storage ? storage->name : "—";
}

