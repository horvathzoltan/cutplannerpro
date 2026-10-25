#pragma once

#include <QGridLayout>


#include "worksheetsummary/model/worksheetorderinfo.h"


class WorkSheetOrderRowWidget
{
public:

    static void insertRow(
        QGridLayout* grid,
        int row,
        const WorkSheetOrderInfo& order,
        const QString& searchText);
};