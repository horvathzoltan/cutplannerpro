#include "worksheetsummarydetailwidget.h"
#include "view/columnindexes/worksheetorderrow_columns.h"
#include "worksheetorderrowwidget.h"
#include <QVBoxLayout>
#include <algorithm>

WorkSheetSummaryDetailWidget::
    WorkSheetSummaryDetailWidget(
        const WorkSheetSummary& summary,
        QWidget* parent)
    : QWidget(parent)
{
    auto* grid =
        new QGridLayout(this);

    grid->setContentsMargins(2,2,2,2);

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

    grid->setColumnMinimumWidth(toColumnIndex(WorkSheetOrderColumn::Ref),60);
    grid->setColumnMinimumWidth(toColumnIndex(WorkSheetOrderColumn::Owner),160);
    grid->setColumnMinimumWidth(toColumnIndex(WorkSheetOrderColumn::Due),120);
    grid->setColumnMinimumWidth(toColumnIndex(WorkSheetOrderColumn::TypeSubtype),180);
    grid->setColumnStretch(toColumnIndex(WorkSheetOrderColumn::Color),1);
    grid->setColumnMinimumWidth(toColumnIndex(WorkSheetOrderColumn::Progress),40);

    for (int row = 0;row < orders.size();++row)
    {
        OrderRowUi ui = WorkSheetOrderRowWidget::insertRow(grid,row,orders[row]);
        _orderRows.insert(orders[row].id,ui);
    }
}

namespace
{

void applyHighlight( QWidget* w, const QString& originalStyle)
{
    if (!w) return;

    w->setStyleSheet( originalStyle + ";background:#704040;");
}

}


void WorkSheetSummaryDetailWidget::applySearchResult(const SearchResult& result)
{
    for (const auto& hit : result.hits)
    {
        if (hit.target !=
            SearchTarget::Order)
        {
            continue;
        }

        auto it = _orderRows.find( hit.id);

        if (it == _orderRows.end())
        {
            continue;
        }

        const OrderRowUi& ui = it.value();

        if (hit.column < 0 || static_cast<std::size_t>(hit.column) >= ColumnCount)
        {
            continue;
        }

        const auto column = static_cast<WorkSheetOrderColumn>(hit.column);

        applyHighlight(ui.widget(column),ui.style(column));
    }
}

void WorkSheetSummaryDetailWidget:: clearSearchHighlights()
{
    for (auto it =_orderRows.begin();it != _orderRows.end();++it)
    {
        const OrderRowUi& ui = it.value();

        for (std::size_t i = 0; i < ColumnCount; ++i)
        {
            QWidget* w = ui.widgets[i];

            if (!w) continue;
            auto a = static_cast<WorkSheetOrderColumn>( static_cast<int>(i));

            qDebug()
                << "clearSearchHighlights: stored_style:"
                << ui.style(a);

            w->setStyleSheet(ui.style(a));
        }
    }
}