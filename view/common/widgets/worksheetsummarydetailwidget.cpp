#include "worksheetsummarydetailwidget.h"

#include "worksheetorderrowwidget.h"

#include <QVBoxLayout>

#include <algorithm>

WorkSheetSummaryDetailWidget::
    WorkSheetSummaryDetailWidget(
        const WorkSheetSummary& summary,
        const QString& searchText,
        QWidget* parent)
    : QWidget(parent)
{
    auto* layout =
        new QVBoxLayout(this);

    layout->setContentsMargins(
        2,
        2,
        2,
        2);

    layout->setSpacing(1);

    QVector<WorkSheetOrderInfo> orders =
        summary.orders;

    std::sort(
        orders.begin(),
        orders.end(),
        [](const WorkSheetOrderInfo& a,
           const WorkSheetOrderInfo& b)
        {
            return a.dueDate < b.dueDate;
        });

    for (const auto& order : orders)
    {
        layout->addWidget(
            new WorkSheetOrderRowWidget(
                order,
                searchText,
                this));
    }

    layout->addStretch();
}