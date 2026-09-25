#include "materials/registry/material_rolegroup_registry.h"
#include "common/logger.h"
#include "material_registry.h"

#include <materials/repository/material_storagegrouprepository.h>

MaterialRoleGroupRegistry& MaterialRoleGroupRegistry::instance() {
    static MaterialRoleGroupRegistry registry;
    return registry;
}

void MaterialRoleGroupRegistry::registerGroup(const MaterialRoleGroup& group) {
    _data[group.id] = group;
    _barcodeToGroup[group.barcode] = group.id;

    for (const auto& matId : group.members()) {
        _materialToGroup[matId] = group.id;
    }
}

void MaterialRoleGroupRegistry::clearAll() {
    _data.clear();
    _materialToGroup.clear();
    _barcodeToGroup.clear();
}

QList<MaterialRoleGroup> MaterialRoleGroupRegistry::readAll() const {
    return _data.values();
}

const MaterialRoleGroup* MaterialRoleGroupRegistry::findById(const QUuid& groupId) const {
    auto it = _data.find(groupId);
    return it != _data.end() ? &it.value() : nullptr;
}

const MaterialRoleGroup* MaterialRoleGroupRegistry::findByMaterialId(const QUuid& materialId) const {
    auto it = _materialToGroup.find(materialId);
    if (it != _materialToGroup.end()) {
        return findById(it.value());
    }
    return nullptr;
}

bool MaterialRoleGroupRegistry::containsBarcode(const QString& barcode) const {
    return _barcodeToGroup.contains(barcode);
}

const MaterialRoleGroup* MaterialRoleGroupRegistry::findByBarcode(const QString& barcode) const {
    auto it = _barcodeToGroup.find(barcode);
    if (it == _barcodeToGroup.end()) return nullptr;
    return findById(it.value());
}

void MaterialRoleGroupRegistry::debugDump() const
{
    zInfo("==============================================");
    zInfo("🔍 MaterialRoleGroupRegistry dump indul");
    zInfo("==============================================");

    if (_data.isEmpty()) {
        zWarning("⚠️ Nincsenek szerepkör-csoportok a registry-ben.");
        return;
    }

    for (const auto& group : _data) {

        zInfo(QString("📦 Csoport: %1 (%2)")
                  .arg(group.name)
                  .arg(group.barcode));

        const auto& members = group.members();
        if (members.isEmpty()) {
            zWarning("   ⚠️ Nincsenek tagok.");
            continue;
        }

        zInfo("   Tagok:");
        for (const QUuid& matId : members) {
            const auto* mat = MaterialRegistry::instance().findById(matId);
            if (mat) {
                zInfo(QString("      ➤ %1 [%2]")
                          .arg(mat->name)
                          .arg(mat->barcode));
            } else {
                zWarning(QString("      ➤ ⚠️ Ismeretlen anyag ID: %1")
                             .arg(matId.toString()));
            }
        }

        zInfo(""); // üres sor a csoportok között
    }

    zInfo("==============================================");
    zInfo("🔚 MaterialRoleGroupRegistry dump vége");
    zInfo("==============================================");
}
