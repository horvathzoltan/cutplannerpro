#pragma once

#include "materials/model/material_master.h"
#include "materials/utils/material_group_utils.h"
#include "view/tablerowstyler/tablestyleutils.h"

namespace MaterialRowStyler {
inline void applyMaterialStyle(QTableWidget* table, int row, const MaterialMaster* mat, const QSet<int>& excols) {
    if (!table || !mat) return;

    QColor backColor = GroupUtils::groupColor(mat->id); // vagy mat->color.toQColor()
    QColor textColor = backColor.lightness() < 128 ? Qt::white : Qt::black;

    if(mat->isBundle())
        backColor = QColor("#9B59B6");   // soft purple

    for (int col = 0; col < table->columnCount(); ++col) {
        if(excols.contains(col))
            continue;
        TableStyleUtils::setCellStyle(table, row, col, backColor, textColor);
    }
}
} // endof namespace MaterialRowStyler
