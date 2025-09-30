#pragma once

#include <QVector>
#include <QUuid>
//#include <optional>
#include "../storageentry.h"

class StorageRegistry {
private:
    StorageRegistry() = default;
    QVector<StorageEntry> _data;

        void collectChildrenRecursive(const QUuid& parentId, QStringList& out) const;
public:
    static StorageRegistry& instance();

    // 📥 Betöltés kívülről (pl. repositoryból)
    void setData(const QVector<StorageEntry>& data);

    // 📤 Elérés
    QVector<StorageEntry> readAll() const { return _data; }

    // 🔍 Keresés egyedi azonosítóval
    const StorageEntry* findById(const QUuid& id) const;

    // 🔍 Keresés parentId alapján (fa-nézethez)
    QVector<StorageEntry> findByParentId(const QUuid& parentId) const;

    // 🔄 Teljes törlés (UI reset esetén pl.)
    void clearAll();
    const StorageEntry* findByBarcode(const QString &barcode) const;

    // 🆕 Root + children (rekurzív) lekérdezés
    QStringList resolveTargetStoragesRecursive(const QUuid& rootId) const;
};
