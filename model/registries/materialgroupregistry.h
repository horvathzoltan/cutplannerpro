#pragma once

#include "../materialgroup.h"
#include <QUuid>
#include <QMap>
#include <QList>

// 🔗 Anyagcsoportok tárolója: lekérdezhető, singleton
class MaterialGroupRegistry {
private:
    MaterialGroupRegistry() = default;
    MaterialGroupRegistry(const MaterialGroup&) = delete;

    // QMap használata a gyors és kulcs-alapú elérés érdekében (groupId → MaterialGroup).
    // Ez lehetővé teszi az anyagcsoportok és anyagok hatékony keresését UUID alapján.
    // A sorrend nem számít, az adatintegritás és a lekérdezhetőség a prioritás.

    QMap<QUuid, MaterialGroup> _data;     // groupId → group
    QMap<QUuid, QUuid> _materialToGroup;  // materialId → groupId

public:
    static MaterialGroupRegistry& instance();

    void registerGroup(const MaterialGroup& group);
    void clearAll();

    QList<MaterialGroup> readAll() const;
    const MaterialGroup* findById(const QUuid& groupId) const;
    const MaterialGroup* findByMaterialId(const QUuid& materialId) const;

    bool isEmpty() const { return _data.isEmpty(); }
};
