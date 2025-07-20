#pragma once

#include <QString>
#include <QUuid>
#include <QList>

// 🔗 Egy csoport, amely logikailag összetartozó anyagokat tartalmaz
struct MaterialGroup {
    QUuid groupId;                  // 🔐 Egyedi azonosító (GUID)
    QString name;                   // 🏷️ Csoportnév (UI-ban megjeleníthető)
    QList<QUuid> materialIds;       // 📦 Hozzátartozó anyagok GUID-ja
    QString colorHex;               // 🎨 Szín hex kóddal (pl. "#AABBCC")


    bool contains(const QUuid& id) const {
        return materialIds.contains(id);
    }

    int size() const {
        return materialIds.size();
    }
};
