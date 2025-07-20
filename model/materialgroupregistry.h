#pragma once

#include "materialgroup.h"
#include <QUuid>
#include <QMap>
#include <QList>

// 🔗 Anyagcsoportok tárolója: lekérdezhető, singleton
class MaterialGroupRegistry {
public:
    static MaterialGroupRegistry& instance();

    void addGroup(const MaterialGroup& group);
    void clear();

    const MaterialGroup* findByGroupId(const QUuid& groupId) const;
    const MaterialGroup* findByMaterialId(const QUuid& materialId) const;
    QList<MaterialGroup> all() const;

private:
    QMap<QUuid, MaterialGroup> _groups;          // groupId → csoport
    QMap<QUuid, QUuid> _materialToGroup;         // materialId → groupId
};
