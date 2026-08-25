#pragma once

#include "leftover/model/leftoverstockentry.h"
#include "service/cutting/instruction/labelmodel.h"

namespace LeftoverLabelGenerator{

inline LabelModel makeBundleLeftoverLabel(
    const LeftoverStockEntry& e,
    const QString& parentRodBarcode)
{
    LabelModel lm;

    lm.priorityIcon = "";
    lm.groupIcon = "";

    const MaterialMaster* mat =
        MaterialRegistry::instance().findById(e.materialId);

    QString materialCode = mat ? mat->barcode : "???";
    QString materialName = mat? mat->toDisplay() : "???";

    // komponensek összesítése
    // QMap<QUuid, int> compCount;
    // for (const auto& bc : e.bundleComponentLengths)
    //     compCount[bc.materialId]++;

    // QString compLine;
    // for (auto it = compCount.begin(); it != compCount.end(); ++it) {
    //     const MaterialMaster* cm =
    //         MaterialRegistry::instance().findById(it.key());
    //     QString name = cm ? cm->barcode : it.key().toString();
    //     if (!compLine.isEmpty())
    //         compLine += " + ";
    //     compLine += QString("%1 × %2").arg(name).arg(it.value());
    // }

    // hossz (trimmed)
    int trimmed = (e.availableLength_mm / 25) * 25;

    // vonalkód tartalom
    lm.barcode = e.barcode;

    // FŐSOR: rúd → leftoverBarcode : trimmedLength mm
    QString mainLine = QString(" | ➨ %1: %2 mm")
                           .arg(e.barcode)
                           .arg(trimmed);

    lm.parts.append({
        parentRodBarcode,
        false, false,
        0, Qt::AlignCenter,
        false, false, false
    });

    lm.parts.append({
        mainLine,
        false, false,
        0, Qt::AlignRight,
        false, false, false
    });

    // ANYAG – vastag, alsó sor
    lm.parts.append({
        materialName,
        false, false,
        1, Qt::AlignCenter,
        false, true, false
    });

    // KOMPONENSEK – meta sor
    // lm.parts.append({
    //     compLine,
    //     false, false,
    //     2, Qt::AlignLeft,
    //     true, false, true
    // });

    // KOMPONENSEK – meta sor
    // lm.parts.append({
    //     compLine,
    //     false, false,
    //     2, Qt::AlignLeft,
    //     true, false, false
    // });

    // // PARENT – meta sor
    // lm.parts.append({
    //     QString("parent=%1").arg(parentRodBarcode),
    //     false, false,
    //     3, Qt::AlignLeft,
    //     true, false, true
    // });


    return lm;
}

} // endof namespace