#pragma once

#include "materials/model/material_paintgroup.h"
#include <QUuid>
#include <QMap>
#include <QList>

// 🎨 Festési csoportok tárolója: lekérdezhető, singleton
class MaterialPaintGroupRegistry {
private:
    MaterialPaintGroupRegistry() = default;
    MaterialPaintGroupRegistry(const MaterialPaintGroup&) = delete;

    // groupId → MaterialPaintGroup
    QMap<QUuid, MaterialPaintGroup> _data;

    // materialId → groupId
    QMap<QUuid, QUuid> _materialToGroup;

    // barcode → groupId
    QMap<QString, QUuid> _barcodeToGroup;

public:
    static MaterialPaintGroupRegistry& instance();

    void registerGroup(const MaterialPaintGroup& group);
    void clearAll();

    QList<MaterialPaintGroup> readAll() const;

    const MaterialPaintGroup* findById(const QUuid& groupId) const;
    const MaterialPaintGroup* findByMaterialId(const QUuid& materialId) const;

    bool isEmpty() const { return _data.isEmpty(); }
    bool containsBarcode(const QString& barcode) const;
    const MaterialPaintGroup* findByBarcode(const QString& barcode) const;

    void debugDump() const;
};
