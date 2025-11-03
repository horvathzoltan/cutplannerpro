#pragma once
#include "model/registries/materialregistry.h"
#include "model/material/materialgroup_utils.h"
#include "view/viewmodels/tablecellviewmodel.h"

namespace CellGenerators {

inline TableCellViewModel materialCell(const QUuid& materialId,
                                       const QString& barcode,
                                       const QColor& baseColor,
                                       const QColor& fgColor)
{
    const auto* mat = MaterialRegistry::instance().findById(materialId);
    QString materialName = mat ? mat->name : "Ismeretlen";
    QString groupName = GroupUtils::groupName(materialId);

    // Szöveg: Material [Group] : Barcode
    QString text = groupName.isEmpty()
                       ? materialName
                       : QString("%1 [%2]").arg(materialName, groupName);

    if (!barcode.isEmpty())
        text.append(QString(" : %1").arg(barcode));

    // Tooltip: részletesebb info
    QString tooltip = QString("Anyag: %1\nCsoport: %2\nBarcode: %3")
                          .arg(materialName,
                               groupName.isEmpty() ? "—" : groupName,
                               barcode.isEmpty() ? "—" : barcode);

    return TableCellViewModel::fromText(text, tooltip, baseColor, fgColor);
}

} // namespace CellGenerators
