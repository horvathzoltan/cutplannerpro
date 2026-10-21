#include "worksheetorderrowwidget.h"

#include "common/color/namedcolor.h"

#include <product/registry/product_subtype_registry.h>
#include <product/registry/product_type_registry.h>

#include <QHBoxLayout>
#include <QLabel>

namespace
{

bool matchesText(
    const WorkSheetOrderInfo& order,
    const QString& searchText)
{
    QString txt =
        searchText.trimmed();

    if (txt.isEmpty())
    {
        return false;
    }

    auto* type =
        ProductTypeRegistry::instance()
            .findById(order.productTypeId);

    auto* subtype =
        ProductSubtypeRegistry::instance()
            .findById(order.productSubtypeId);

    QString typeName =
        type
            ? type->name
            : QString();

    QString subtypeName =
        subtype
            ? subtype->name
            : QString();

    return
        order.externalReference.contains(
            txt,
            Qt::CaseInsensitive)
        ||
        order.ownerName.contains(
            txt,
            Qt::CaseInsensitive)
        ||
        order.requiredColorCode.contains(
            txt,
            Qt::CaseInsensitive)
        ||
        typeName.contains(
            txt,
            Qt::CaseInsensitive)
        ||
        subtypeName.contains(
            txt,
            Qt::CaseInsensitive);
}

QLabel* createLabel(
    const QString& text)
{
    auto* lbl =
        new QLabel(text);

    lbl->setTextInteractionFlags(
        Qt::TextSelectableByMouse);

    return lbl;
}

}

WorkSheetOrderRowWidget::
    WorkSheetOrderRowWidget(
        const WorkSheetOrderInfo& order,
        const QString& searchText,
        QWidget* parent)
    : QWidget(parent)
{
    auto* layout =
        new QHBoxLayout(this);

    layout->setContentsMargins(
        4,
        2,
        4,
        2);

    layout->setSpacing(8);

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

    QLabel* lblDue =
        createLabel(
            order.dueDate.toString(
                "yyyy.MM.dd"));

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

    QLabel* colorBox =
        new QLabel();

    colorBox->setFixedSize(
        12,
        12);

    NamedColor color =
        NamedColor::fromUserInput(
            order.requiredColorCode);

    if (color.isValid())
    {
        colorBox->setStyleSheet(
            QString(
                "background-color:%1;"
                "border-radius:5px;"
                "border:1px solid #888;")
                .arg(
                    color.color().name()));

        colorBox->setToolTip(
            color.code());
    }
    else
    {
        colorBox->hide();
    }

    layout->addWidget(
        lblRef);

    layout->addWidget(
        lblOwner);

    layout->addWidget(
        lblDue);

    layout->addWidget(
        lblType);

    layout->addWidget(
        colorBox);

    layout->addWidget(
        lblProgress);

    layout->addStretch();

    if (matchesText(
            order,
            searchText))
    {
        setStyleSheet(
            "background:#704040;");
    }

    setAutoFillBackground(true);
}