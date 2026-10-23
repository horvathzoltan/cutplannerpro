#include "worksheetorderrowwidget.h"

#include "common/color/namedcolor.h"
#include "common/emojihelper.h"
#include "materials/view/material_cell_generator.h"

#include <product/registry/product_subtype_registry.h>
#include <product/registry/product_type_registry.h>

#include <QHBoxLayout>
#include <QLabel>

#include <view/viewmodels/tablecellviewmodel.h>
#include "worksheetsummary/worksheetsearch_helper.h"

namespace
{


QLabel* createLabel(
    const QString& text)
{
    auto* lbl = new QLabel(text);

    QFont f = lbl->font();

    f.setPointSize(
        std::max(
            7,
            f.pointSize() - 2));

    f.setItalic(true);

    lbl->setFont(f);

    lbl->setTextInteractionFlags(
        Qt::TextSelectableByMouse);

    return lbl;
}

void applyHighlight(QWidget* w)
{
    if (!w)
        return;

    w->setStyleSheet(
        "background:#704040;");
};


}

void WorkSheetOrderRowWidget::insertRow(
        QGridLayout* grid,
        int row,
        const WorkSheetOrderInfo& order,
        const QString& searchText)
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

    grid->addWidget(lblRef, row, 0);
    grid->addWidget(lblOwner, row, 1);
    grid->addWidget(lblDue, row, 2);
    grid->addWidget(lblType, row, 3);

    if (colorVm.widget)
    {
        auto f = lblRef->font();
        f.setItalic(true);
        colorVm.widget->setToolTip(colorVm.tooltip);
        //colorVm.widget->layout()->setAlignment(Qt::AlignLeft);
        colorVm.widget->setFont(f);

        grid->addWidget(colorVm.widget, row, 4);
    }

    grid->addWidget(lblProgress, row, 5);


    if (WorkSheetSearchHelper::matches(order,searchText))
    {
        //setStyleSheet("background:#704040;");
        applyHighlight(lblRef);
        applyHighlight(lblOwner);
        applyHighlight(lblDue);
        applyHighlight(lblType);
        applyHighlight(lblProgress);
        applyHighlight(colorVm.widget);
    }

    //setAutoFillBackground(true);
}