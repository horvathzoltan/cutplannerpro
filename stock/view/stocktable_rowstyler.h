#pragma once

#include "common/audit/agestyleutils.h"
#include "materials/view/material_row_styler.h"
#include "stock/view//stocktable_manager.h"
#include "view/tableutils/colorlogicutils.h"

namespace StockTable{
namespace RowStyler{

inline void applyStyle(QTableWidget* table, int row, int length_mm, int quantity, const MaterialMaster* mat, const QDateTime& lastSeenAt)
{
    if (!table) return;

    QColor backColor1 = ColorLogicUtils::colorForLength(length_mm);
    TableStyleUtils::setCellStyle(table, row, StockTableManager::ColLength, backColor1, Qt::black);
    QColor backColor2 = ColorLogicUtils::colorForQuantity(quantity);
    TableStyleUtils::setCellStyle(table, row, StockTableManager::ColQuantity, backColor2, Qt::black);
    AgeTableUtils::applyAgeStyle(table,row, StockTableManager::ColLastSeenAt, AgeLogicUtils::determine(lastSeenAt));

    MaterialRowStyler::applyMaterialStyle(table, row, mat,
                                          {StockTableManager::ColLength,
                                           StockTableManager::ColQuantity,
                                           StockTableManager::ColLastSeenAt});

}

} // endof namespace RowStyler
} // endof namespace StockTable
