#pragma once

#include <QString>
#include "identifiableentity.h"
#include "storagetype.h"


// 🔖 Raktári bejegyzés, egyszerű típusinformációval
struct StorageEntry : public IdentifiableEntity {
    QUuid parentId; // 🔗 Szülő azonosító (pl. raktár vagy polc)

    StorageType type;        // 📌 Bejegyzés típusa (pl. "Rod", "Box", "FinishedProduct", stb.)
    QString comment;     // 📝 Opcionális megjegyzés vagy címke

    // 👓 Rövid megjelenítési név: [Típus] Név [Lokáció]
    QString displayName() const {
      //  QString suffix = "[" + type + "]";
        //if (!location.isEmpty()) suffix += " @" + location;
      return name;// + " " + suffix;
    }

    // 🔧 Teljes technikai leírás
    // QString toString() const {
    //     return QString("StorageEntry{id=%1, name=%2, type=%3, location=%4}")
    //     .arg(id.toString(), name, type, location);
    // }
};
