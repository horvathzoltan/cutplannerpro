#include "materialregistry.h"

MaterialRegistry &MaterialRegistry::instance() {
    static MaterialRegistry reg;
    return reg;
}

const MaterialMaster* MaterialRegistry::findById(const QUuid& id) const {
    for (const auto& m : _data)
        if (m.id == id)
            return &m;
    return nullptr;
}

const MaterialMaster* MaterialRegistry::findByBarcode(const QString& barcode) const {
    for (const auto& m : _data)
        if (m.barcode == barcode)
            return &m;
    return nullptr;
}

bool MaterialRegistry::isBarcodeUnique(const QString& barcode) const {
    for (const auto& m : _data)
        if (m.barcode == barcode)
            return false;
    return true;
}

bool MaterialRegistry::registerData(const MaterialMaster& material) {
    if (!isBarcodeUnique(material.barcode))
        return false;
    _data.append(material);
    return true;
}


