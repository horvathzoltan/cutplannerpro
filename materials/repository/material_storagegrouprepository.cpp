#include "materials/repository/material_storagegrouprepository.h"
#include "materials/registry/material_storagegroupregistry.h"
#include "materials/registry/material_rolegroup_registry.h"
#include "materials/registry/material_registry.h"
#include "materialbundles/registry/bundle_registry.h"
#include "common/logger.h"

#include <QSet>

#include <product/registry/material_role_registry.h>

QMap<QUuid, QUuid> MaterialStorageGroupRepository::buildStorageGroups()
{
    auto& reg = MaterialStorageGroupRegistry::instance();
    reg.clearAll();

    QMap<QUuid, QUuid> sourceToStorage;   // 🔥 EZ A MAP

    const auto& mats = MaterialRegistry::instance().readAll();
    const auto& roleGroups = MaterialRoleGroupRegistry::instance().readAll();

    for (const auto& rg : roleGroups)
    {
        QSet<QUuid> members;

        // 🔥 1) Gyártási csoport taganyagok → alap tagok
        for (const QUuid& matId : rg.members())
        {
            members.insert(matId);

            const auto* m = MaterialRegistry::instance().findById(matId);
            if (!m) continue;

            // 🔥 2) Ha a taganyag bundle → robbantás
            if (m->hasBundle())
            {
                auto comps = BundleRegistry::instance().componentsOf(m->bundleCode);
                for (const auto& comp : comps)
                    members.insert(comp.materialId);
            }
        }

        // 🔥 3) Bundle ANYAGOK felvétele (nem csak komponensek!)
        for (const auto& m : mats)
        {
            if (!m.hasBundle())
                continue;

            const auto* bundle = BundleRegistry::instance().findByCode(m.bundleCode);
            if (!bundle)
                continue;

            // Ha bármely bundle komponens a gyártási csoport tagja → a bundle ANYAG is tag
            for (const auto& comp : bundle->components)
            {
                if (rg.members().contains(comp.materialId))
                {
                    members.insert(m.id);
                    break;
                }
            }
        }

        // 🔥 4) Tárolási csoport létrehozása
        QString storageBarcode = "SG-" + rg.barcode;
        QString storageName = QString("Tárolási %1").arg(rg.name);

        MaterialStorageGroup sg(
            QUuid::createUuid(),
            storageName,
            storageBarcode,
            members.values().toVector()
            );

        reg.registerGroup(sg);

        // 🔥 A MAP FELTÖLTÉSE
        sourceToStorage[rg.id] = sg.id;
    }

    zInfo("StorageRoleGroupRepository: tárolási csoportok felépítve (bundleCode + bundle komponensek alapján).");

    return sourceToStorage;
}

