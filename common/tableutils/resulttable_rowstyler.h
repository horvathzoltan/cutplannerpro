#pragma once

#include <QTableWidget>
#include <model/cutresult.h>
#include "../../model/materialmaster.h"
#include "common/materialutils.h"
#include "model/cutplan.h"

// 🎨 Táblázatsor stíluskezelő – közös utility
namespace ResultTable {
namespace RowStyler {

inline void applyStyle(QTableWidget* table, int row,
                                 const MaterialMaster* mat,
                                 const CutPlan& plan) {
    if (!table || !mat)
        return;

    QColor bg = MaterialUtils::colorForMaterial(*mat);
    QColor fg = bg.lightness() < 128 ? Qt::white : Qt::black;

    QString groupName = GroupUtils::groupName(mat->id);
    QString tooltip = QString("Anyag: %1\nCsoport: %2\nBarcode: %3")
                          .arg(mat->name)
                          .arg(groupName.isEmpty() ? "Nincs csoport" : groupName)
                          .arg(mat->barcode);

    for (int col = 0; col < table->columnCount(); ++col) {
        QTableWidgetItem* item = table->item(row, col);
        if (!item) {
            item = new QTableWidgetItem();
            table->setItem(row, col, item);
        }

        item->setBackground(bg);
        item->setForeground(fg);
        item->setTextAlignment(Qt::AlignCenter);

        switch (col) {
        case 0:
            item->setToolTip(QString("Szál sorszáma: %1\n%2").arg(plan.rodNumber).arg(tooltip));
            break;
        case 3:
            item->setToolTip(QString("Fűrészszélesség összesen: %1 mm").arg(plan.kerfTotal));
            break;
        case 4:
            item->setToolTip(QString("Hulladék: %1 mm").arg(plan.waste));
            break;
        default:
            item->setToolTip(tooltip);
            break;
        }
    }
}

} // endof namespace RowStyler
} // endof namespace name
