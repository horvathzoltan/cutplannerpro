#pragma once

#include "materialbundles/model/bundle_definition.h"
#include <QMap>
#include <QList>
#include <QUuid>

class BundleRegistry {
private:
    BundleRegistry() = default;
    BundleRegistry(const BundleRegistry&) = delete;

    QMap<QUuid, BundleDefinition> _byId;      // bundleId → bundle
    QMap<QString, QUuid>          _byCode;    // code → bundleId

public:
    static BundleRegistry& instance();

    void registerBundle(const BundleDefinition& def);
    void clearAll();

    QList<BundleDefinition> readAll() const;

    const BundleDefinition* findById(const QUuid& id) const;
    const BundleDefinition* findByCode(const QString& code) const;

    bool isEmpty() const { return _byId.isEmpty(); }

    QMap<QUuid, int> computeComponentNeed(const QString &bundleCode, int strandCount) const;
    QVector<BundleComponent> componentsOf(const QString &bundleCode) const;
};
