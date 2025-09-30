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
QStringList StorageRegistry::resolveTargetStoragesRecursive(const QUuid& rootId) const {
    QStringList result;

    // Root maga
    if (auto rootOpt = findById(rootId)) {
        result << rootOpt->name;
    }

    // Gyerekek rekurzívan
    collectChildrenRecursive(rootId, result);

    return result;
}

void StorageRegistry::collectChildrenRecursive(const QUuid& parentId, QStringList& out) const {
    const auto& children = findByParentId(parentId);
    for (const auto& child : children) {
        out << child.name;
        collectChildrenRecursive(child.id, out); // mélyebb szintek
    }
}
