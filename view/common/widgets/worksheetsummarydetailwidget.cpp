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
    auto* grid =
        new QGridLayout(this);

    grid->setContentsMargins(
        2,
        2,
        2,
        2);

    grid->setSpacing(1);

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

    // for (const auto& order : orders)
    // {
    //     grid->addWidget(
    //         new WorkSheetOrderRowWidget(
    //             order,
    //             searchText,
    //             this));
    // }

    grid->setColumnMinimumWidth(0, 60);   // ref
    grid->setColumnMinimumWidth(1, 160);  // owner
    grid->setColumnMinimumWidth(2, 120);  // due
    grid->setColumnMinimumWidth(3, 180);  // type

    grid->setColumnStretch(4, 1);         // color

    grid->setColumnMinimumWidth(5, 40);   // progress


    for (int row = 0;
         row < orders.size();
         ++row)
    {
        WorkSheetOrderRowWidget::insertRow(
            grid,
            row,
            orders[row],
            searchText);
    }

    //layout->addStretch();
}