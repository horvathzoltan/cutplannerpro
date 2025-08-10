#pragma once

#include "common/tableutils/colorlogicutils.h"
#include "common/tableutils/tablestyleutils.h"
#include <view/managers/stocktablemanager.h>

namespace StockTable{
namespace RowStyler{

inline void applyStyle(QTableWidget* table, int row, int length_mm, int quantity, QColor baseColor)
{
    if (!table) return;

    constexpr int ColLength = StockTableManager::ColLength;
    constexpr int ColQuantity = StockTableManager::ColQuantity;
    QColor textColor = Qt::black;

    for (int col = 0; col < table->columnCount(); ++col) {

        QColor backColor;
        if (col == ColLength)
            backColor = ColorLogicUtils::colorForLength(length_mm);
        else if (col == ColQuantity)
            backColor = ColorLogicUtils::colorForQuantity(quantity);
        else
            backColor = baseColor;

        TableStyleUtils::setCellStyle(table, row, col, backColor, textColor);
    }

}
} // endof namespace RowStyler
} // endof namespace StockTable
