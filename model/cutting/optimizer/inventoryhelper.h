#pragma once

#include <QVector>
#include <materials/registry/material_registry.h>
#include <model/leftoverstockentry.h>
#include "../../../common/logger.h"
//#include "model/registries/materialregistry.h"
#include "model/registries/storageregistry.h"

namespace Cutting {
namespace Optimizer {

class InventoryHelper
{
public:
    static inline void logSnapshot(const QVector<LeftoverStockEntry>& leftovers)
    {
        zInfo("📦 INVENTORY SNAPSHOT — újrahasznosítható hullók");

        for (const auto& e : leftovers) {
            const MaterialMaster* mat =
                MaterialRegistry::instance().findById(e.materialId);

            const StorageEntry* st =
                StorageRegistry::instance().findById(e.storageId);

            zInfo(QString("   • Hulló: barcode=%1, hossz=%2 mm, anyag=%3, tároló=%4, forrás=%5")
                      .arg(e.barcode)
                      .arg(e.availableLength_mm)
                      .arg(mat ? mat->toDisplay() : e.materialId.toString())
                      .arg(st ? st->name : "ismeretlen")
                      .arg(e._parent.has_value()
                               ? QString("parent=%1").arg(e._parent->toString())
                               : "parent=—"));
        }

        zInfo("📘 SNAPSHOT — vége");
    }
};

} // namespace Optimizer
} // namespace Cutting
