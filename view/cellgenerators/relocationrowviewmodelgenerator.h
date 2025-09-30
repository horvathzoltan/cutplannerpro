#pragma once

#include "common/tableutils/colorlogicutils.h"
#include "model/relocation/relocationinstruction.h"
#include "view/columnindexes/relocationplantable_columns.h"
#include "view/viewmodels/tablerowviewmodel.h"
#include "view/viewmodels/tablecellviewmodel.h"
#include "view/cellhelpers/cellfactory.h"

#include <QColor>
#include <QObject>

namespace RelocationRowViewModelGenerator {

// /// 🔹 Helper: egyszerű szöveges cella létrehozása
// inline TableCellViewModel createTextCell(const QString& text,
//                                          const QString& tooltip = {},
//                                          const QColor& background = Qt::white,
//                                          const QColor& foreground = Qt::black,
//                                          bool isReadOnly = true) {
//     TableCellViewModel cell;
//     cell.text = text;
//     cell.tooltip = tooltip;
//     cell.background = background;
//     cell.foreground = foreground;
//     cell.isReadOnly = isReadOnly;
//     return cell;
// }

/// 🔹 Teljes TableRowViewModel generálása egy RelocationInstruction alapján
inline TableRowViewModel generate(const RelocationInstruction& instr,
                                  const MaterialMaster* mat,
                                  QObject* /*receiver*/ = nullptr) {
    TableRowViewModel vm;
    //vm.rowId = instr.rowId;

    // 🎨 Alapszínek a csoport alapján
    QColor baseColor = ColorLogicUtils::resolveBaseColor(mat);
    QColor fgColor = baseColor.lightness() < 128 ? Qt::white : Qt::black;

    // Anyag
    vm.cells[RelocationPlanTableColumns::Material] =
        CellFactory::textCell(instr.materialName,
                       QString("Anyag: %1").arg(instr.materialName),
                       baseColor, fgColor);

    // Vonalkód
    vm.cells[RelocationPlanTableColumns::Barcode] =
        CellFactory::textCell(instr.barcode,
                       QString("Vonalkód: %1").arg(instr.barcode),
                       baseColor, fgColor);

    // Mennyiség / státusz
    QString qtyText = instr.isSatisfied ? QStringLiteral("✔ Megvan")
                                        : QString::number(instr.plannedQuantity);
    QColor qtyColor = instr.isSatisfied ? QColor("#228B22")
                                        : (instr.plannedQuantity == 0 ? QColor("#B22222") : fgColor);

    vm.cells[RelocationPlanTableColumns::Quantity] =
        CellFactory::textCell(qtyText,
                       QString("Mennyiség: %1").arg(instr.plannedQuantity),
                       baseColor, qtyColor);

    // Forrás
    vm.cells[RelocationPlanTableColumns::Source] =
        CellFactory::textCell(instr.sourceLocation,
                       QString("Forrás: %1").arg(instr.sourceLocation),
                       baseColor, fgColor);

    // Cél
    vm.cells[RelocationPlanTableColumns::Target] =
        CellFactory::textCell(instr.targetLocation,
                       QString("Cél: %1").arg(instr.targetLocation),
                       baseColor, fgColor);

    // Típus
    QString typeText = (instr.sourceType == AuditSourceType::Stock) ? "📦 Stock" : "♻️ Hulló";
    vm.cells[RelocationPlanTableColumns::Type] =
        CellFactory::textCell(typeText,
                       QString("Forrás típusa: %1").arg(typeText),
                       baseColor, fgColor);

    return vm;
}

} // namespace RelocationRowViewModelGenerator
