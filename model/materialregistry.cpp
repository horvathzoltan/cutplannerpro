#include "materialregistry.h"

const MaterialMaster* MaterialRegistry::findById(const QUuid& id) const {
    for (const auto& m : materials)
        if (m.id == id)
            return &m;
    return nullptr;
}

const MaterialMaster* MaterialRegistry::findByBarcode(const QString& barcode) const {
    for (const auto& m : materials)
        if (m.barcode == barcode)
            return &m;
    return nullptr;
}

bool MaterialRegistry::isBarcodeUnique(const QString& barcode) const {
    for (const auto& m : materials)
        if (m.barcode == barcode)
            return false;
    return true;
}

bool MaterialRegistry::insert(const MaterialMaster& material) {
    if (!isBarcodeUnique(material.barcode))
        return false;
    materials.append(material);
    return true;
}

void MaterialRegistry::setMaterials(const QVector<MaterialMaster>& newMaterials) {
    materials = newMaterials;
}
