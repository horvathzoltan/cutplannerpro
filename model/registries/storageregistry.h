#pragma once

#include <QVector>
#include <QUuid>
#include <QMap>
//#include <optional>
#include "../storageentry.h"

class StorageRegistry {
private:
    StorageRegistry() = default;
    QVector<StorageEntry> _data;
    QMap<QUuid, QString> _uniqueNameCache;

    void collectChildrenRecursive(const QUuid& parentId, QStringList& out) const;
    bool isUniqueCandidate(const QString& candidate, const QUuid& selfId, int depth) const;
    QString buildCandidate(const StorageEntry *s, int depth) const;

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
    QStringList getNamesRecursive(const QUuid& rootId) const;
    QVector<StorageEntry> getRecursive(const QUuid &rootId) const;
    void collectChildrenRecursive(const QUuid &parentId, QVector<StorageEntry> &out) const;
    bool isDescendantOf(const QUuid &childId, const QUuid &ancestorId) const;
    QString uniqueHumanName(const QUuid &id);
    //bool isUnique(const QString &name);
};
