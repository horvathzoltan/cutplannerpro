#pragma once

#include "materials/utils/material_utils.h"
#include "../tablerowviewmodel.h"
#include "../tablecellviewmodel.h"
#include "../../../model/cutting/plan/cutplan.h"
#include "materials/utils/material_group_utils.h"
//#include "view/tableutils/colorlogicutils.h"
//#include "model/material/material_utils.h"

#include <QLabel>
#include <QHBoxLayout>
#include <QWidget>

#include "../../../model/registries/cuttingplanrequestregistry.h"
#include "materials/registry/material_registry.h"

namespace Results::ViewModel::BadgeRowGenerator {

inline TableRowViewModel generate(const Cutting::Plan::CutPlan& plan) {
    TableRowViewModel vm;
    vm.rowId = QUuid::createUuid();

    QString groupName = GroupUtils::groupName(plan.materialId);
    QColor baseColor = GroupUtils::groupColor(plan.materialId);
    //QColor baseColor = MaterialUtils::colorForMaterial(plan.materialId);
    //const auto* mat = MaterialRegistry::instance().findById(plan.materialId);
    //QColor baseColor = mat ? ColorLogicUtils::resolveBaseColor(mat) : QColor("#DDDDDD");


    QWidget* cutsWidget = new QWidget;
    QHBoxLayout* layout = new QHBoxLayout(cutsWidget);
    layout->setContentsMargins(0, 4, 0, 4);
    layout->setSpacing(6);

    cutsWidget->setAutoFillBackground(true);
    cutsWidget->setStyleSheet(QString("background-color: %1;"
                                      "padding-top: 6px; padding-bottom: 6px;")
                                  .arg(baseColor.name()));
    for (const Cutting::Segment::SegmentModel& s : plan.segments) {
        QString color;
        if (s.isPiece()) {
            color = (s.length_mm() < 300)  ? "#e74c3c"   // rövid
                    : (s.length_mm() > 2000) ? "#f39c12"   // hosszú
                                             : "#27ae60";  // normál
        }
        else if (s.isKerf()) {
            color = "#34495e";
        }
        else if (s.isWaste()) {
            color = "#bdc3c7";
        }
        else if (s.isTechnical()) {
            color = "#7f8c8d";
        }

        QString badgeLabel = s.toLabelString();

        const auto* req = CuttingPlanRequestRegistry::instance().findById(s._requestId);


        QString badgeTooltip = QString(
                                   "Szálazonosító: %1\n"
                                   "Barcode: %2\n"
                                   "Material: %3\n"
                                   "Csoport: %4\n"
                                   "CutSize: %5 mm\n"
                                   "Remaining: %6 mm\n"
                                   "Parent: %7\n"
                                   "Forrás: %8\n"
                                   "OptimizationId: %9\n"
                                   "Gép: %10\n"
                                   "Státusz: %11\n"
                                   )
                                   .arg(plan.rodId)
                                   .arg(s.barcode())
                                   .arg(plan.materialName())
                                   .arg(groupName.isEmpty() ? "Nincs csoport" : groupName)
                                   .arg(s.length_mm())
                                   .arg(s.isWaste() ? QString::number(s.length_mm()) : "—")
                                   .arg(plan.parentBarcode.value_or("—"))
                                   .arg(plan.source == Cutting::Plan::Source::Stock ? "Stock"
                                        : plan.source == Cutting::Plan::Source::Reusable ? "Reusable"
                                                                                         : "Optimization")
                                   .arg(plan.optimizationId)
                                   .arg(plan.machineName)
                                   .arg(Cutting::Plan::statusText(plan.status))
                                   ;
        if (s.isPiece()) {
            badgeTooltip = QString("Tételszám: %1.\n\n").arg(s.externalReference) + badgeTooltip;
        }

        if(req){
            QString rodTooltip;
            rodTooltip = CuttingPlanRequestUtils::cuttingPlanRequestToDisplay(*req,DisplayType::Tooltip);
            badgeTooltip += "\nRequest:"+rodTooltip;
        }

        QLabel* label = new QLabel(badgeLabel);
        label->setToolTip(badgeTooltip);
        label->setAlignment(Qt::AlignCenter);
        label->setStyleSheet(QString(
                                 "QLabel { background-color: %1; color: white; font-weight: bold; padding: 3px 8px; border-radius: 6px; }"
                                 ).arg(color));

        if (s.isKerf())
            label->setFixedWidth(60);
        else if (s.isTechnical())
            label->setFixedWidth(60);

        layout->addWidget(label);
    }

    vm.cells[0] = TableCellViewModel::fromWidget(cutsWidget, "Vágási szegmensek");

    return vm;
}

} // namespace Results::ViewModel::BadgeRowGenerator
