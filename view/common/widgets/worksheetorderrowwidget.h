#pragma once

#include <array>

#include <QGridLayout>
#include <QLabel>
#include <QPointer>
#include <QWidget>

#include "view/columnindexes/worksheetorderrow_columns.h"
#include "worksheetsummary/model/worksheetorderinfo.h"

struct OrderRowUi
{
    std::array<QPointer<QWidget>, ColumnCount> widgets {};

    std::array<QString, ColumnCount> originalStyles;

    QWidget* widget( WorkSheetOrderColumn column) const
    {
        return widgets[toColumnIndex(column)];
    }

    QString& style(WorkSheetOrderColumn column)
    {
        return originalStyles[toColumnIndex(column)];
    }

    const QString& style(WorkSheetOrderColumn column) const
    {
        return originalStyles[toColumnIndex(column)];
    }
};

class WorkSheetOrderRowWidget
{
public:

    static OrderRowUi insertRow(
        QGridLayout* grid,
        int row,
        const WorkSheetOrderInfo& order);
};