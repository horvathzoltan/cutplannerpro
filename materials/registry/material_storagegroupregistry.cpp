#include "materials/registry/material_storagegroupregistry.h"
#include "materials/registry/material_registry.h"
#include "common/logger.h"

MaterialStorageGroupRegistry& MaterialStorageGroupRegistry::instance()
{
    static MaterialStorageGroupRegistry inst;
    return inst;
}

void MaterialStorageGroupRegistry::clearAll()
{
    _data.clear();
    _materialToGroup.clear();
    _barcodeToGroup.clear();
}

void MaterialStorageGroupRegistry::registerGroup(const MaterialStorageGroup& group)
{
    _data[group.id] = group;
    _barcodeToGroup[group.barcode] = group.id;

    for (const QUuid& matId : group.members)
        _materialToGroup[matId] = group.id;
}

QList<MaterialStorageGroup> MaterialStorageGroupRegistry::readAll() const
{
    return _data.values();
}

const MaterialStorageGroup* MaterialStorageGroupRegistry::findById(const QUuid& id) const
{
    auto it = _data.find(id);
    return it != _data.end() ? &it.value() : nullptr;
}

const MaterialStorageGroup* MaterialStorageGroupRegistry::findByMaterialId(const QUuid& matId) const
{
    auto it = _materialToGroup.find(matId);
    if (it == _materialToGroup.end())
        return nullptr;

    return findById(it.value());
}

const MaterialStorageGroup* MaterialStorageGroupRegistry::findByBarcode(const QString& bc) const
{
    auto it = _barcodeToGroup.find(bc);
    if (it == _barcodeToGroup.end())
        return nullptr;

    return findById(it.value());
}

bool MaterialStorageGroupRegistry::containsBarcode(const QString& bc) const
{
    return _barcodeToGroup.contains(bc);
}

void MaterialStorageGroupRegistry::debugDump() const
{
    zInfo("==============================================");
    zInfo("🔍 StorageRoleGroupRegistry dump indul");
    zInfo("==============================================");

    if (_data.isEmpty()) {
        zWarning("⚠️ Nincsenek tárolási anyagcsoportok.");
        return;
    }

    for (const auto& group : _data) {

        zInfo(QString("📦 Tárolási csoport: %1 (%2)")
                  .arg(group.name)
                  .arg(group.barcode));

        if (group.members.isEmpty()) {
            zWarning("   ⚠️ Nincsenek tagok.");
            continue;
        }

        zInfo("   Tagok:");
        for (const QUuid& matId : group.members) {
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

        zInfo("");
    }

    zInfo("==============================================");
    zInfo("🔚 StorageRoleGroupRegistry dump vége");
    zInfo("==============================================");
}
