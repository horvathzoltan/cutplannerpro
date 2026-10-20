#include "worksheetsummarytable_manager.h"
#include "../tableutils/tableutils.h"
#include "worksheetsummary/registry/worksheetsummary_registry.h"
#include <QTableWidgetItem>
#include <algorithm>
#include <product/registry/product_subtype_registry.h>
#include <product/registry/product_type_registry.h>

namespace
{

QString buildDetailText(
    const WorkSheetSummary& summary)
{
    if (summary.orders.isEmpty())
    {
        return "Nincs rendelési információ.";
    }

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

    QStringList lines;

    for (const auto& order : orders)
    {

        auto* type = ProductTypeRegistry::instance().findById(order.productTypeId);
        auto* subtype = ProductSubtypeRegistry::instance().findById(order.productSubtypeId);

        QString typeName = type ? type->name : "";
        QString subtypeName = subtype ? subtype->name : "";
        QString colorName = order.requiredColorCode.isEmpty() ? "N/A" : order.requiredColorCode;

        lines << QString(
                     "%1 %2 | %3 | %4/%5")
                     .arg(order.externalReference+".", -7)
                     .arg(order.ownerName,       -25)
                     .arg(
                         QString("%1/%2/%3")
                             .arg(typeName)
                             .arg(subtypeName)
                             .arg(colorName),
                         -30)
                     .arg(order.completedCount)
                     .arg(order.requestCount);
    }

    return lines.join("\n");
}

}


bool WorkSheetSummaryTableManager::_isVerbose = false;

WorkSheetSummaryTableManager::WorkSheetSummaryTableManager(
    QTableWidget* table, QWidget* parent)
    : QObject(parent), _table(table), parent(parent)
{
}

void WorkSheetSummaryTableManager::addRow(
    const WorkSheetSummary& summary)
{
    if (!_table)
        return;

    int rowIx =
        _table->rowCount();

    _table->insertRow(rowIx);

    _table->setItem(
        rowIx,
        0,
        new QTableWidgetItem(
            summary.fileName));

    _table->setItem(
        rowIx,
        1,
        new QTableWidgetItem(
            WorkSheetPresenceUtils::toDisplayText(summary.presence)));

    _table->setItem(
        rowIx,
        2,
        new QTableWidgetItem(
            QString::number(
                summary.requestCount())));

    _table->setItem(
        rowIx,
        3,
        new QTableWidgetItem(
            QString::number(
                summary.completedCount())));

    _table->setItem(
        rowIx,
        4,
        new QTableWidgetItem(
            QString::number(
                summary.uncompletedCount())));

    _table->setItem(
        rowIx,
        5,
        new QTableWidgetItem(
            QString::number(
                summary.undefinedCount())));
    _table->setItem(
        rowIx,
        6,
        new QTableWidgetItem(
            summary.requiresAttention()? "Yes" : "No")
        );

    if (_isVerbose)
    {
        qDebug()
            << "Worksheet row added:"
            << summary.fileName;
    }

    if (!summary.orders.isEmpty())
    {
        int detailRowIx =
            _table->rowCount();

        _table->insertRow(detailRowIx);

        QString detailText =
            buildDetailText(summary);

        auto* item =
            new QTableWidgetItem(
                detailText);

        QFont font =
            item->font();

        font.setPointSize(
            std::max(
                8,
                font.pointSize() - 2));

        item->setFont(font);

        int lineCount =
            detailText.count('\n') + 1;

        QFontMetrics fm(font);

        int rowHeight =
            lineCount * fm.lineSpacing() + 8;

        rowHeight =
            std::max(
                24,
                rowHeight);

        _table->setRowHeight(
            detailRowIx,
            rowHeight);

        item->setForeground(
            QColor("#666666"));

        _table->setItem(
            detailRowIx,
            0,
            item);

        _table->setSpan(
            detailRowIx,
            0,
            1,
            _table->columnCount());
    }
}

void WorkSheetSummaryTableManager::
    refresh_TableFromRegistry()
{
    if (!_table)
        return;

    TableUtils::clearSafely(
        _table);

    const auto& data =
        WorkSheetSummaryRegistry::instance()
            .readAll();

    for (const auto& item : data)
    {
        addRow(item);
    }
}

void WorkSheetSummaryTableManager::clearTable()
{
    if (!_table)
        return;

    _table->setRowCount(0);
    _table->clearContents();
}