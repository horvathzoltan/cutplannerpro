#include "storageregistry.h"

StorageRegistry& StorageRegistry::instance() {
    static StorageRegistry reg;
    return reg;
}

void StorageRegistry::setData(const QVector<StorageEntry>& data) {
    _data = data;
    _uniqueNameCache.clear();   // ⭐ invalidate cache
}

void StorageRegistry::clearAll() {
    _data.clear();
    _uniqueNameCache.clear();   // ⭐ invalidate cache
}

const StorageEntry* StorageRegistry::findById(const QUuid& id) const {
    for (const auto& s : _data) {
        if (s.id == id)
            return &s;
    }
    return nullptr;
}

QVector<StorageEntry> StorageRegistry::findByParentId(const QUuid& parentId) const {
    QVector<StorageEntry> result;
    for (const auto& s : _data) {
        if (s.parentId == parentId)
            result.append(s);
    }
    return result;
}

const StorageEntry* StorageRegistry::findByBarcode(const QString& barcode) const {
    for (const auto& s : _data) {
        if (s.barcode == barcode)
            return &s;
    }
    return nullptr;
}

// storageregistry.cpp
QStringList StorageRegistry::getNamesRecursive(const QUuid& rootId) const {

    auto a = getRecursive(rootId);
    QStringList result;

    for (const auto& entry : a) {
        result.append(entry.name);
    }
    return result;
}

QVector<StorageEntry> StorageRegistry::getRecursive(const QUuid& rootId) const {
    QVector<StorageEntry> result;

    if (auto rootOpt = findById(rootId)) {
        result.append(*rootOpt);
    }

    collectChildrenRecursive(rootId, result);
    return result;
}

void StorageRegistry::collectChildrenRecursive(const QUuid& parentId, QVector<StorageEntry>& out) const {
    const auto& children = findByParentId(parentId);
    for (const auto& child : children) {
        out.append(child);
        collectChildrenRecursive(child.id, out);
    }
}

bool StorageRegistry::isDescendantOf(const QUuid& childId, const QUuid& ancestorId) const {
    auto current = findById(childId);
    while (current) {
        if (current->parentId == ancestorId)
            return true;
        current = findById(current->parentId);
    }
    return false;
}

QString StorageRegistry::uniqueHumanName(const QUuid& id)
{
    // 1) Cache hit
    if (_uniqueNameCache.contains(id))
        return _uniqueNameCache[id];

    const StorageEntry* s = findById(id);
    if (!s) return "—";

    int depth = 1;

    while (true) {
        QString candidate = buildCandidate(s, depth);

        if (isUniqueCandidate(candidate, id, depth)){
            _uniqueNameCache[id] = candidate;   // ⭐ cache store
            return candidate;
            }

        depth++;

        // fallback: teljes path
        if (depth > 10){   // soha nem lesz ilyen mély
            _uniqueNameCache[id] = candidate;   // ⭐ cache store
            return candidate;
            }
    }
}

bool StorageRegistry::isUniqueCandidate(const QString& candidate, const QUuid& selfId, int depth) const
{
    int count = 0;

    for (const auto& s : _data) {
        QString cand = buildCandidate(&s, depth);
        if (cand == candidate && s.id != selfId)
            count++;
    }

    return count == 0;
}


QString StorageRegistry::buildCandidate(const StorageEntry* s, int depth) const
{
    QStringList parts;
    const StorageEntry* cur = s;

    for (int i = 0; i < depth && cur; ++i) {
        parts.prepend(cur->name);
        cur = findById(cur->parentId);
    }

    return parts.join(" / ");
}


