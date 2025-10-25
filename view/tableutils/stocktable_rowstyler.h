#pragma once

#include "view/tablerowstyler/materialrowstyler.h"
#include "view/tableutils/colorlogicutils.h"
#include "view/tablerowstyler/tablestyleutils.h"
#include "view/managers/stocktable_manager.h"

namespace StockTable{
namespace RowStyler{

inline void applyStyle(QTableWidget* table, int row, int length_mm, int quantity, const MaterialMaster* mat)
{
    if (!table) return;

    constexpr int ColLength = StockTableManager::ColLength;
    constexpr int ColQuantity = StockTableManager::ColQuantity;
    QColor textColor = Qt::black;

    // for (int col = 0; col < table->columnCount(); ++col) {

    //     QColor backColor;
    //     if (col == ColLength){
    //         Qcolor backColor1 = ColorLogicUtils::colorForLength(length_mm);
    //         TableStyleUtils::setCellStyle(table, row, col, backColor, textColor);
    //     }
    //     else if (col == ColQuantity){
    //         backColor = ColorLogicUtils::colorForQuantity(quantity);
    //         TableStyleUtils::setCellStyle(table, row, col, backColor, textColor);
    //     }

    // }

    QColor backColor1 = ColorLogicUtils::colorForLength(length_mm);
    TableStyleUtils::setCellStyle(table, row, ColLength, backColor1, textColor);
    QColor backColor2 = ColorLogicUtils::colorForQuantity(quantity);
    TableStyleUtils::setCellStyle(table, row, ColQuantity, backColor2, textColor);

    MaterialRowStyler::applyMaterialStyle(table, row, mat,{ColLength, ColQuantity});

}
} // endof namespace RowStyler
} // endof namespace StockTable
