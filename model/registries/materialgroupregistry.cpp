#include "materialgroupregistry.h"

MaterialGroupRegistry& MaterialGroupRegistry::instance() {
    static MaterialGroupRegistry registry;
    return registry;
}

void MaterialGroupRegistry::addGroup(const MaterialGroup& group) {
    _groups[group.groupId] = group;
    for (const auto& id : group.materialIds) {
        _materialToGroup[id] = group.groupId;
    }
}

void MaterialGroupRegistry::clear() {
    _groups.clear();
    _materialToGroup.clear();
}

const MaterialGroup* MaterialGroupRegistry::findByGroupId(const QUuid& groupId) const {
    auto it = _groups.find(groupId);
    return it != _groups.end() ? &it.value() : nullptr;
}

const MaterialGroup* MaterialGroupRegistry::findByMaterialId(const QUuid& materialId) const {
    auto it = _materialToGroup.find(materialId);
    if (it != _materialToGroup.end()) {
        return findByGroupId(it.value());
    }
    return nullptr;
}

QList<MaterialGroup> MaterialGroupRegistry::all() const {
    return _groups.values();
}
