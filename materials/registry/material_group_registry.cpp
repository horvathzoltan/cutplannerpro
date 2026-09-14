#include "materials/registry/material_group_registry.h"

MaterialGroupRegistry& MaterialGroupRegistry::instance() {
    static MaterialGroupRegistry registry;
    return registry;
}

void MaterialGroupRegistry::registerGroup(const MaterialGroup& group) {
    _data[group.id] = group;
    _barcodeToGroup[group.barcode] = group.id;

    for (const auto& id : group.materialIds) {
        _materialToGroup[id] = group.id;
    }
}

void MaterialGroupRegistry::clearAll() {
    _data.clear();
    _materialToGroup.clear();
    _barcodeToGroup.clear();
}

const MaterialGroup* MaterialGroupRegistry::findById(const QUuid& groupId) const {
    auto it = _data.find(groupId);
    return it != _data.end() ? &it.value() : nullptr;
}

const MaterialGroup* MaterialGroupRegistry::findByMaterialId(const QUuid& materialId) const {
    auto it = _materialToGroup.find(materialId);
    if (it != _materialToGroup.end()) {
        return findById(it.value());
    }
    return nullptr;
}

QList<MaterialGroup> MaterialGroupRegistry::readAll() const {
    return _data.values();
}

bool MaterialGroupRegistry::containsBarcode(const QString& barcode) const {
    return _barcodeToGroup.contains(barcode);
}

const MaterialGroup* MaterialGroupRegistry::findByBarcode(const QString& barcode) const {
    auto it = _barcodeToGroup.find(barcode);
    if (it == _barcodeToGroup.end()) return nullptr;
    return findById(it.value());
}
