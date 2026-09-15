#pragma once
#include "materials/model/material_storagegroup.h"
#include <QMap>
#include <QUuid>

class MaterialStorageGroupRegistry
{
private:
    MaterialStorageGroupRegistry() = default;

    // groupId → StorageRoleGroup
    QMap<QUuid, MaterialStorageGroup> _data;

    // materialId → groupId
    QMap<QUuid, QUuid> _materialToGroup;

    // barcode → groupId
    QMap<QString, QUuid> _barcodeToGroup;

public:
    static MaterialStorageGroupRegistry& instance();

    void clearAll();

    void registerGroup(const MaterialStorageGroup& group);

    QList<MaterialStorageGroup> readAll() const;

    const MaterialStorageGroup* findById(const QUuid& id) const;
    const MaterialStorageGroup* findByMaterialId(const QUuid& matId) const;
    const MaterialStorageGroup* findByBarcode(const QString& bc) const;

    bool containsBarcode(const QString& bc) const;

    void debugDump() const;
};
