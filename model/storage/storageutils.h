#pragma once
#include "model/registries/storageregistry.h"
#include <QString>
#include <QStringList>
#include <QUuid>

namespace StorageUtils {

inline QString buildPathTree(const QUuid& id) {
    const StorageRegistry& reg = StorageRegistry::instance();

    QStringList parts;
    QUuid current = id;

    while (!current.isNull()) {
        const StorageEntry* e = reg.findById(current);
        if (!e)
            break;

        parts.prepend(e->name);
        current = e->parentId;
    }

    // Ha csak egy elem van → nincs hierarchia
    if (parts.size() <= 1)
        return parts.join("");

    // Hierarchia rajzolása
    QString out = parts.first() + "\n";

    for (int i = 1; i < parts.size(); ++i) {
        QString indent = QString(" ").repeated((i - 1) * 4);
        out += indent + "└── " + parts[i] + "\n";
    }

    return out.trimmed();
}

inline bool isDescendantOf(const QUuid& child, const QUuid& root) {
    if (child.isNull() || root.isNull())
        return false;

    const StorageRegistry& reg = StorageRegistry::instance();
    QUuid current = child;

    while (!current.isNull()) {
        if (current == root)
            return true;

        const StorageEntry* e = reg.findById(current);
        if (!e)
            break;

        current = e->parentId;
    }

    return false;
}

}
