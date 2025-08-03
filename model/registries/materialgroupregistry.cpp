#include "materialgroupregistry.h"

MaterialGroupRegistry& MaterialGroupRegistry::instance() {
    static MaterialGroupRegistry registry;
    return registry;
}

void MaterialGroupRegistry::registerGroup(const MaterialGroup& group) {
    _data[group.groupId] = group;
    for (const auto& id : group.materialIds) {
        _materialToGroup[id] = group.groupId;
    }
}

void MaterialGroupRegistry::clearAll() {
    _data.clear();
    _materialToGroup.clear();
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
