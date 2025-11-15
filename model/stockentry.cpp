#include "stockentry.h"
#include "materials/registry/material_registry.h"

#include "materials/utils/material_group_utils.h"

#include "registries/storageregistry.h"

const MaterialMaster* StockEntry::master() const {
    if (materialId.isNull())
        return nullptr;

    const MaterialMaster *opt = MaterialRegistry::instance().findById(materialId);
    return opt;
}

const StorageEntry* StockEntry::storage() const {
    if (materialId.isNull())
        return nullptr;

    const StorageEntry* storage = StorageRegistry::instance().findById(storageId);
    return storage;
}

QString StockEntry::materialName() const {
    const auto* m = master();
    return m ? m->name : "(?)";
}

QString StockEntry::materialBarcode() const {
    const auto* m = master();
    return m ? m->barcode : "(?)";
}

MaterialType StockEntry::materialType() const {
    const auto* m = master();
    return m ? m->type : MaterialType(MaterialType::Type::Unknown);
}

QString StockEntry::materialGroupName() const {
    return GroupUtils::groupName(materialId);
}

QColor StockEntry::materialGroupColor() const {
    return GroupUtils::groupColor(materialId);
}

QString StockEntry::storageName() const {
    const auto* m = storage();
    return m ? m->name : "(?)";
}

QString StockEntry::storageBarcode() const {
    const auto* m = storage();
    return m ? m->barcode : "(?)";
}
