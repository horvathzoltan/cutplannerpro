#pragma once

#include "worksheetsummary/model/worksheetorderinfo.h"

#include <QGridLayout>


class WorkSheetOrderRowWidget
{
public:

    static void insertRow(
        QGridLayout* grid,
        int row,
        const WorkSheetOrderInfo& order,
        const QString& searchText);
};