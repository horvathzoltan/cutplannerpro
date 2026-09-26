#pragma once
#include "leftover/model/leftoverstockentry.h"
#include "materials/registry/material_registry.h"

inline QString buildLeftoverBundleTooltip(const LeftoverStockEntry& e)
{
    QStringList lines;

    lines << QString("♻️ Leftover bundle");
    lines << QString("Barcode: %1").arg(e.barcode);
    lines << QString("Elérhető hossz: %1 mm").arg(e.availableLength_mm);

    if (e.bundleComponentLengths.isEmpty()) {
        //lines << "  (nincs bundle komponens – sima leftover)";
        return lines.join("\n");
    }

    lines << "Komponensek:";


    for (const BundleComponentLength& c : e.bundleComponentLengths)
    {
        const MaterialMaster* mat =
            MaterialRegistry::instance().findById(c.materialId);

        QString name    = mat ? mat->name    : "?";
        QString barcode = mat ? mat->barcode : "?";

        QString lenInfo;

        if (c.length_mm == 0) {
            lenInfo = "nem tartalmazza";
        }
        else if (c.length_mm == -1) {
            lenInfo = QString("leftover hossz érvényes")
                          .arg(e.availableLength_mm);
        }
        else {
            lenInfo = QString("%1 mm").arg(c.length_mm);
        }

        lines << QString("  • %1 [%2]   %3")
                     .arg(name)
                     .arg(barcode)
                     .arg(lenInfo);
    }

    return lines.join("\n");
}
