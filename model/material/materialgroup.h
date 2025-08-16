#pragma once

#include <QString>
#include <QUuid>
#include <QList>

#include "model/identifiableentity.h"

// 🔗 Egy csoport, amely logikailag összetartozó anyagokat tartalmaz
struct MaterialGroup  : public IdentifiableEntity {
//    QUuid groupId;                  // 🔐 Egyedi azonosító (GUID)
//    QString name;                   // 🏷️ Csoportnév (UI-ban megjeleníthető)
//    QString barcode;
    QList<QUuid> materialIds;       // 📦 Hozzátartozó anyagok GUID-ja
    QString colorHex;               // 🎨 Szín hex kóddal (pl. "#AABBCC")

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
};
