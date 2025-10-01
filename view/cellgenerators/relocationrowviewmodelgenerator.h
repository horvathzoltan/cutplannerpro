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
/// 🔹 Teljes TableRowViewModel generálása egy RelocationInstruction alapján
inline TableRowViewModel generate(const RelocationInstruction& instr,
                                  const MaterialMaster* mat,
                                  QObject* /*receiver*/ = nullptr) {
    TableRowViewModel vm;
    // vm.rowId = instr.rowId; // ha később kell egyedi azonosító

    // 🎨 Alapszínek a csoport alapján
    QColor baseColor = ColorLogicUtils::resolveBaseColor(mat);
    QColor fgColor   = baseColor.lightness() < 128 ? Qt::white : Qt::black;

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
    QString qtyText = instr.isSatisfied
                          ? QStringLiteral("✔ Megvan")
                          : QString::number(instr.plannedQuantity);

    QColor qtyColor = instr.isSatisfied
                          ? QColor("#228B22")   // zöld, ha teljesült
                          : (instr.plannedQuantity == 0
                                 ? QColor("#B22222") // piros, ha 0
                                 : fgColor);

    vm.cells[RelocationPlanTableColumns::Quantity] =
        CellFactory::textCell(qtyText,
                              QString("Terv szerinti mennyiség: %1").arg(instr.plannedQuantity),
                              baseColor, qtyColor);

    // Forrás lista → "hely1 (moved/available), hely2 (moved/available)"
    QStringList sourceParts;
    for (const auto& src : instr.sources) {
        sourceParts << QString("%1 (%2/%3)")
        .arg(src.locationName)
            .arg(src.moved)
            .arg(src.available);
    }
    QString sourceText = sourceParts.isEmpty() ? "—" : sourceParts.join(", ");

    vm.cells[RelocationPlanTableColumns::Source] =
        CellFactory::textCell(sourceText,
                              QString("Forrás tárhelyek: %1").arg(sourceText),
                              baseColor, fgColor);

    // Cél lista → "hely1 (placed), hely2 (placed)"
    QStringList targetParts;
    for (const auto& tgt : instr.targets) {
        targetParts << QString("%1 (%2)")
        .arg(tgt.locationName)
            .arg(tgt.placed);
    }
    QString targetText = targetParts.isEmpty() ? "—" : targetParts.join(", ");

    vm.cells[RelocationPlanTableColumns::Target] =
        CellFactory::textCell(targetText,
                              QString("Cél tárhelyek: %1").arg(targetText),
                              baseColor, fgColor);

    // Típus
    QString typeText = (instr.sourceType == AuditSourceType::Stock)
                           ? QStringLiteral("📦 Stock")
                           : QStringLiteral("♻️ Hulló");

    vm.cells[RelocationPlanTableColumns::Type] =
        CellFactory::textCell(typeText,
                              QString("Forrás típusa: %1").arg(typeText),
                              baseColor, fgColor);

    return vm;
}


} // namespace RelocationRowViewModelGenerator
