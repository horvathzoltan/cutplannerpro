#include "materialbundles/registry/bundle_registry.h"
#include "common/logger.h"

BundleRegistry& BundleRegistry::instance() {
    static BundleRegistry reg;
    return reg;
}

void BundleRegistry::registerBundle(const BundleDefinition& def) {
    _byId[def.id] = def;
    _byCode[def.code] = def.id;
}

void BundleRegistry::clearAll() {
    _byId.clear();
    _byCode.clear();
}

const BundleDefinition* BundleRegistry::findById(const QUuid& id) const {
    auto it = _byId.find(id);
    return it != _byId.end() ? &it.value() : nullptr;
}

const BundleDefinition* BundleRegistry::findByCode(const QString& code) const {
    auto it = _byCode.find(code);
    if (it == _byCode.end()) return nullptr;
    return findById(it.value());
}

QList<BundleDefinition> BundleRegistry::readAll() const {
    return _byId.values();
}


QMap<QUuid, int> BundleRegistry::computeComponentNeed(const QString& bundleCode,
                                                      int strandCount) const
{
    QMap<QUuid, int> need;

    const BundleDefinition* def = findByCode(bundleCode);
    if (!def) {
        zWarning(QStringLiteral("⚠️ computeComponentNeed: Unknown bundleCode: %1")
                     .arg(bundleCode));
        return need;
    }

    for (const auto& comp : def->components) {
        need[comp.materialId] += comp.count * strandCount;
    }

    return need;
}

QVector<BundleComponent> BundleRegistry::componentsOf(const QString& bundleCode) const
{
    const BundleDefinition* def = findByCode(bundleCode);
    if (!def) {
        zWarning(QStringLiteral("⚠️ componentsOf: Unknown bundleCode: %1").arg(bundleCode));
        return {};
    }
    return def->components;
}
