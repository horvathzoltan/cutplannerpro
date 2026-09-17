#pragma once

#include "materials/model/material_rolegroup.h"
#include <QUuid>
#include <QMap>
#include <QList>

// 🔗 Anyag-szerepkör csoportok tárolója: lekérdezhető, singleton
class MaterialRoleGroupRegistry {
private:
    MaterialRoleGroupRegistry() = default;
    MaterialRoleGroupRegistry(const MaterialRoleGroup&) = delete;

    // groupId → MaterialRoleGroup
    QMap<QUuid, MaterialRoleGroup> _data;

    // materialId → groupId
    QMap<QUuid, QUuid> _materialToGroup;

    // barcode → groupId
    QMap<QString, QUuid> _barcodeToGroup;

public:
    static MaterialRoleGroupRegistry& instance();

    void registerGroup(const MaterialRoleGroup& group);
    void clearAll();

    QList<MaterialRoleGroup> readAll() const;

    const MaterialRoleGroup* findById(const QUuid& groupId) const;
    const MaterialRoleGroup* findByMaterialId(const QUuid& materialId) const;

    bool isEmpty() const { return _data.isEmpty(); }
    bool containsBarcode(const QString& barcode) const;
    const MaterialRoleGroup* findByBarcode(const QString& barcode) const;
    void debugDump() const;
};
