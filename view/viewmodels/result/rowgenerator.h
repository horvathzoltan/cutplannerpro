#pragma once

#include "materials/view/material_cell_generator.h"
#include "../tablerowviewmodel.h"
#include "../tablecellviewmodel.h"
#include "../../columnindexes/tableresults_columns.h"
#include "../../../model/cutting/plan/cutplan.h"
#include "materials/registry/material_registry.h"

namespace Results::ViewModel::RowGenerator {

inline TableRowViewModel generate(const Cutting::Plan::CutPlan& plan) {
    TableRowViewModel vm;
    vm.rowId = QUuid::createUuid();

    const MaterialMaster* mat = MaterialRegistry::instance().findById(plan.materialId);
    //QColor baseColor = mat ? MaterialUtils::colorForMaterial(*mat) : QColor("#cccccc");
    //QColor fgColor = baseColor.lightness() < 128 ? Qt::white : Qt::black;

    auto matCell = CellGenerators::materialCell(*mat, plan.sourceBarcode);

    QColor baseColor = matCell.background;
    QColor fgColor = matCell.foreground;

    vm.cells[ResultsTableColumns::Material] = matCell;


    // QString groupName = GroupUtils::groupName(plan.materialId);
    // QString materialName = plan.materialName();
    // QString categoryText = groupName.isEmpty()
    //                            ? materialName
    //                            : groupName + " | " + materialName;

    // 🧩 RodId
    //QString rodLabel = QString("%1|%2").arg(plan.rodId, plan.sourceBarcode);
    QString rodTooltip = QString("RodId: %1\n"
                                 "Barcode: %2\n"
                                 "Parent: %3\n"
                                 "Forrás: %4\n"
                                 "OptimizationId: %5")
                             .arg(plan.rodId)
                             .arg(plan.sourceBarcode.isEmpty() ? "—" : plan.sourceBarcode)
                             .arg(plan.parentBarcode.value_or("—"))
                             .arg(plan.source == Cutting::Plan::Source::Stock ? "Stock"
                                  : plan.source == Cutting::Plan::Source::Reusable ? "Reusable"
                                                                                   : "Optimization")
                             .arg(plan.optimizationId);

    vm.cells[ResultsTableColumns::RodId] =
        TableCellViewModel::fromText(plan.rodId, rodTooltip, baseColor, fgColor);

    // 🧩 Barcode
    // vm.cells[ResultsTableColumns::Barcode] =
    //     TableCellViewModel::fromText(plan.sourceBarcode.isEmpty() ? "—" : plan.sourceBarcode,
    //                                  QString("Forrás barcode: %1").arg(plan.sourceBarcode.isEmpty() ? "—" : plan.sourceBarcode),
    //                                  baseColor, fgColor);

    // 🧩 Category
    // vm.cells[ResultsTableColumns::Category] =
    //     TableCellViewModel::fromText(categoryText,
    //                                  QString("Anyag: %1\nCsoport: %2").arg(materialName, groupName.isEmpty() ? "—" : groupName),
    //                                  baseColor, fgColor);

    // 📏 Length
    vm.cells[ResultsTableColumns::Length] =
        TableCellViewModel::fromText(QString::number(plan.totalLength),
                                     "Teljes rúd hossz (mm)",
                                     baseColor, fgColor);

    // 📏 Kerf
    vm.cells[ResultsTableColumns::Kerf] =
        TableCellViewModel::fromText(QString::number(plan.kerfTotal),
                                     "Összes kerf (mm)",
                                     baseColor, fgColor);

    // 📏 Waste
    vm.cells[ResultsTableColumns::Waste] =
        TableCellViewModel::fromText(QString::number(plan.waste),
                                     "Hulladék (mm)",
                                     baseColor, fgColor);

    return vm;
}

} // namespace Results::ViewModel::RowGenerator
