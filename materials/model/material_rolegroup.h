#pragma once

#include <QString>
#include <QUuid>
#include <QList>

#include "model/identifiableentity.h"

// 🔗 Egy csoport, amely logikailag összetartozó anyagokat tartalmaz
struct MaterialRoleGroup  : public IdentifiableEntity {

private:
        QList<QUuid> materialIds;       // 📦 Hozzátartozó anyagok GUID-ja

public:
    void addMaterial(const QUuid v){
        if(materialIds.contains(v)) return; // Elkerüljük a duplikációt
        materialIds.append(v);
    }

    bool contains(const QUuid& id) const {
        return materialIds.contains(id);
    }

    int size() const {
        return materialIds.size();
    }

    const QList<QUuid>& members() const{
        return materialIds;
    }
};
