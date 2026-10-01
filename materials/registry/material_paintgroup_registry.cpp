#include "materials/registry/material_paintgroup_registry.h"
#include "common/logger.h"
#include "materials/registry/material_registry.h"

MaterialPaintGroupRegistry& MaterialPaintGroupRegistry::instance() {
    static MaterialPaintGroupRegistry registry;
    return registry;
}

void MaterialPaintGroupRegistry::registerGroup(const MaterialPaintGroup& group) {
    _data[group.id] = group;
    _barcodeToGroup[group.barcode] = group.id;

    for (const auto& matId : group.members()) {
        _materialToGroup[matId] = group.id;
    }
}

void MaterialPaintGroupRegistry::clearAll() {
    _data.clear();
    _materialToGroup.clear();
    _barcodeToGroup.clear();
}

QList<MaterialPaintGroup> MaterialPaintGroupRegistry::readAll() const {
    return _data.values();
}

const MaterialPaintGroup* MaterialPaintGroupRegistry::findById(const QUuid& groupId) const {
    auto it = _data.find(groupId);
    return it != _data.end() ? &it.value() : nullptr;
}

const MaterialPaintGroup* MaterialPaintGroupRegistry::findByMaterialId(const QUuid& materialId) const {
    auto it = _materialToGroup.find(materialId);
    if (it != _materialToGroup.end()) {
        return findById(it.value());
    }
    return nullptr;
}

bool MaterialPaintGroupRegistry::containsBarcode(const QString& barcode) const {
    return _barcodeToGroup.contains(barcode);
}

const MaterialPaintGroup* MaterialPaintGroupRegistry::findByBarcode(const QString& barcode) const {
    auto it = _barcodeToGroup.find(barcode);
    if (it == _barcodeToGroup.end()) return nullptr;
    return findById(it.value());
}

void MaterialPaintGroupRegistry::debugDump() const
{
    zInfo("==============================================");
    zInfo("🎨 MaterialPaintGroupRegistry dump indul");
    zInfo("==============================================");

    if (_data.isEmpty()) {
        zWarning("⚠️ Nincsenek festési csoportok a registry-ben.");
        return;
    }

    for (const auto& group : _data) {

        zInfo(QString("🎨 Csoport: %1 (%2)")
                  .arg(group.name)
                  .arg(group.barcode));

        // Geometria
        zInfo(QString("   ➤ Geometria: %1")
                  .arg(PaintGeometryTypeUtils::toString(group.geometry)));

        // Méretek cm-ben (toString_cm)
        zInfo(QString("   ➤ Méret: %1")
                  .arg(group.toString_cm()));

        // ExtraCut (opcionális)
        if (group.extraCut_mm.has_value()) {
            zInfo(QString("   ➤ ExtraCut: %1 mm").arg(*group.extraCut_mm));
        } else {
            zInfo("   ➤ ExtraCut: -");
        }

        // Tagok
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

        zInfo("");
    }

    zInfo("==============================================");
    zInfo("🔚 MaterialPaintGroupRegistry dump vége");
    zInfo("==============================================");
}

