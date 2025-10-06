#include "storageregistry.h"

StorageRegistry& StorageRegistry::instance() {
    static StorageRegistry reg;
    return reg;
}

void StorageRegistry::setData(const QVector<StorageEntry>& data) {
    _data = data;
}

void StorageRegistry::clearAll() {
    _data.clear();
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

