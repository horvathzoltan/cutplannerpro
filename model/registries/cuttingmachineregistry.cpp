#include "cuttingmachineregistry.h"

CuttingMachineRegistry& CuttingMachineRegistry::instance() {
    static CuttingMachineRegistry reg;
    return reg;
}

bool CuttingMachineRegistry::registerData(const CuttingMachine& machine) {
    if (contains(machine.name))
        return false;
    _data.append(machine);
    return true;
}

const CuttingMachine* CuttingMachineRegistry::findByBarcode(const QString& barcode) const {
    for (const auto& m : _data)
        if (m.barcode == barcode)
            return &m;
    return nullptr;
}

const CuttingMachine* CuttingMachineRegistry::findByName(const QString& name) const {
    for (auto& m : _data)
        if (m.name == name)
            return &m;
    return nullptr;
}

const CuttingMachine* CuttingMachineRegistry::findById(const QUuid& id) const {
    for (auto& m : _data)
        if (m.id == id)
            return &m;
    return nullptr;
}

bool CuttingMachineRegistry::isBarcodeUnique(const QString& barcode) const {
    for (const auto& m : _data)
        if (m.barcode == barcode)
            return false;
    return true;
}

bool CuttingMachineRegistry::contains(const QString& name) const {
    return findByName(name) != nullptr;
}

void CuttingMachineRegistry::clear() {
    _data.clear();
}
