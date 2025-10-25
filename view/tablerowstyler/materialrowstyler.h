#pragma once

#include "model/material/materialmaster.h"
#include "model/material/materialgroup_utils.h"
#include "view/tablerowstyler/tablestyleutils.h"

namespace MaterialRowStyler {
inline void applyMaterialStyle(QTableWidget* table, int row, const MaterialMaster* mat, const QSet<int>& excols) {
    if (!table || !mat) return;

    QColor backColor = GroupUtils::colorForGroup(mat->id); // vagy mat->color.toQColor()
    QColor textColor = backColor.lightness() < 128 ? Qt::white : Qt::black;

    for (int col = 0; col < table->columnCount(); ++col) {
        if(excols.contains(col))
            continue;

        //TableStyleUtils::ensureStyledItem(table, row, col, bg, fg, Qt::AlignCenter, tooltip);
        TableStyleUtils::setCellStyle(table, row, col, backColor, textColor);
    }
}
} // endof namespace MaterialRowStyler
