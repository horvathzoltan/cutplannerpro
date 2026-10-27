#include <QHBoxLayout>
#include <QLabel>

#include "worksheetorderrowwidget.h"


#include "common/color/namedcolor.h"
#include "common/emojihelper.h"
#include "materials/view/material_cell_generator.h"
#include "view/columnindexes/worksheetorderrow_columns.h"
#include <product/registry/product_subtype_registry.h>
#include <product/registry/product_type_registry.h>
#include <view/viewmodels/tablecellviewmodel.h>

namespace
{


QLabel* createLabel(
    const QString& text)
{
    auto* lbl = new QLabel(text);

    QFont f = lbl->font();

    f.setPointSize(
        std::max(7,f.pointSize() - 2));

    f.setItalic(true);

    lbl->setFont(f);

    lbl->setTextInteractionFlags(
        Qt::TextSelectableByMouse);

    return lbl;
}

}

OrderRowUi WorkSheetOrderRowWidget::insertRow(
    QGridLayout* grid,
    int row,
    const WorkSheetOrderInfo& order)
{
    auto* type =
        ProductTypeRegistry::instance()
            .findById(
                order.productTypeId);

    auto* subtype =
        ProductSubtypeRegistry::instance()
            .findById(
                order.productSubtypeId);

    QString typeText =
        type
            ? type->name
            : QString();

    QString subtypeText =
        subtype
            ? subtype->name
            : QString();

    QLabel* lblRef =
        createLabel(
            order.externalReference + ".");

    QLabel* lblOwner =
        createLabel(
            order.ownerName);

    QString dueDateStr = order.dueDate.isValid() ? order.dueDate.toString("yyyy.MM.dd.") : "";
    int daysLeft = QDate::currentDate().daysTo(order.dueDate);
    QString iconKey = EmojiHelper::priorityIconFor_emoji(daysLeft);

    QLabel* lblDue = createLabel( iconKey + " " + dueDateStr);

    QLabel* lblType =
        createLabel(
            QString("%1/%2")
                .arg(typeText)
                .arg(subtypeText));

    QLabel* lblProgress =
        createLabel(
            QString("%1/%2")
                .arg(order.completedCount)
                .arg(order.requestCount));


    const MaterialMaster* mat = MaterialRegistry::instance().findById(order.materialId);

    TableCellViewModel colorVm;

    if (mat)
    {
        colorVm = CellGenerators::requestColorCell(
                order.requiredColor,
                order.requiredSurface,
                mat->color,
                mat->surface,
                mat->paintingMode);
    }
    else
    {
        colorVm =
            TableCellViewModel::fromText(
            "⚠️ Hiányzó anyag",
            "A materialId nem található a MaterialRegistry-ben");
    }



    grid->addWidget(lblRef, row, toColumnIndex(WorkSheetOrderColumn::Ref));
    grid->addWidget(lblOwner, row, toColumnIndex(WorkSheetOrderColumn::Owner));
    grid->addWidget(lblDue, row, toColumnIndex(WorkSheetOrderColumn::Due));
    grid->addWidget(lblType, row, toColumnIndex(WorkSheetOrderColumn::TypeSubtype));

    if (colorVm.hasWidget())
    {

        auto f = lblRef->font();
        auto styleTxt = QString("font-style: italic;font-size: %1pt;").arg(f.pointSize());
        colorVm.setStyleSheet(styleTxt);

        colorVm.apply();

        grid->addWidget(colorVm.widget(), row, toColumnIndex(WorkSheetOrderColumn::Color));
    }

    grid->addWidget(lblProgress, row, toColumnIndex(WorkSheetOrderColumn::Progress));

    OrderRowUi ui;

    ui.widgets[toColumnIndex(WorkSheetOrderColumn::Ref)]= lblRef;
    ui.widgets[toColumnIndex(WorkSheetOrderColumn::Owner)]= lblOwner;
    ui.widgets[toColumnIndex(WorkSheetOrderColumn::Due)] = lblDue;
    ui.widgets[toColumnIndex(WorkSheetOrderColumn::TypeSubtype)] = lblType;
    ui.widgets[toColumnIndex(WorkSheetOrderColumn::Color)] = colorVm.widget();
    ui.widgets[toColumnIndex(WorkSheetOrderColumn::Progress)] = lblProgress;

    ui.originalStyles[toColumnIndex(WorkSheetOrderColumn::Ref)] = lblRef->styleSheet();
    ui.originalStyles[toColumnIndex(WorkSheetOrderColumn::Owner)]= lblOwner->styleSheet();
    ui.originalStyles[toColumnIndex(WorkSheetOrderColumn::Due)] = lblDue->styleSheet();
    ui.originalStyles[toColumnIndex(WorkSheetOrderColumn::TypeSubtype)] = lblType->styleSheet();
    ui.originalStyles[toColumnIndex(WorkSheetOrderColumn::Color)] = colorVm.hasWidget()? colorVm.widget()->styleSheet() : QString();
    ui.originalStyles[toColumnIndex(WorkSheetOrderColumn::Progress)] = lblProgress->styleSheet();

    return ui;
}