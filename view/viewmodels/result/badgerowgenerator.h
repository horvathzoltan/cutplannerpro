#pragma once

#include "view/viewmodels/tablerowviewmodel.h"
#include "view/viewmodels/tablecellviewmodel.h"
#include "model/cutting/plan/cutplan.h"
#include "model/material/materialgroup_utils.h"
#include "view/tableutils/colorlogicutils.h"
//#include "model/material/material_utils.h"

#include <QLabel>
#include <QHBoxLayout>
#include <QWidget>

#include <model/registries/materialregistry.h>

namespace Results::ViewModel::BadgeRowGenerator {

inline TableRowViewModel generate(const Cutting::Plan::CutPlan& plan) {
    TableRowViewModel vm;
    vm.rowId = QUuid::createUuid();

    QString groupName = GroupUtils::groupName(plan.materialId);
    //QColor baseColor = MaterialUtils::colorForMaterial(plan.materialId);
    const auto* mat = MaterialRegistry::instance().findById(plan.materialId);
    QColor baseColor = mat ? ColorLogicUtils::resolveBaseColor(mat) : QColor("#DDDDDD");


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
        switch (s.type()) {
        case Cutting::Segment::SegmentModel::Type::Piece:  color = s.length_mm() < 300 ? "#e74c3c" : s.length_mm() > 2000 ? "#f39c12" : "#27ae60"; break;
        case Cutting::Segment::SegmentModel::Type::Kerf:   color = "#34495e"; break;
        case Cutting::Segment::SegmentModel::Type::Waste:  color = "#bdc3c7"; break;
        }

        QString badgeLabel = s.toLabelString();

        QString badgeTooltip = QString(
                                   "RodId: %1\n"
                                   "Barcode: %2\n"
                                   "Material: %3\n"
                                   "Csoport: %4\n"
                                   "CutSize: %5 mm\n"
                                   "Remaining: %6 mm\n"
                                   "Parent: %7\n"
                                   "Forrás: %8\n"
                                   "OptimizationId: %9\n"
                                   "Gép: %10\n"
                                   "Státusz: %11")
                                   .arg(plan.rodId)
                                   .arg(s.barcode())
                                   .arg(plan.materialName())
                                   .arg(groupName.isEmpty() ? "Nincs csoport" : groupName)
                                   .arg(s.length_mm())
                                   .arg(s.type() == Cutting::Segment::SegmentModel::Type::Waste ? QString::number(s.length_mm()) : "—")
                                   .arg(plan.parentBarcode.value_or("—"))
                                   .arg(plan.source == Cutting::Plan::Source::Stock ? "Stock"
                                        : plan.source == Cutting::Plan::Source::Reusable ? "Reusable"
                                                                                         : "Optimization")
                                   .arg(plan.optimizationId)
                                   .arg(plan.machineName)
                                   .arg(Cutting::Plan::statusText(plan.status));

        QLabel* label = new QLabel(badgeLabel);
        label->setToolTip(badgeTooltip);
        label->setAlignment(Qt::AlignCenter);
        label->setStyleSheet(QString(
                                 "QLabel { background-color: %1; color: white; font-weight: bold; padding: 3px 8px; border-radius: 6px; }"
                                 ).arg(color));

        if (s.type() == Cutting::Segment::SegmentModel::Type::Kerf)
            label->setFixedWidth(60);

        layout->addWidget(label);
    }

    vm.cells[0] = TableCellViewModel::fromWidget(cutsWidget, "Vágási szegmensek");

    return vm;
}

} // namespace Results::ViewModel::BadgeRowGenerator
