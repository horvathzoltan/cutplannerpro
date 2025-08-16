#pragma once

#include "common/grouputils.h"
//#include "common/tableutils/colorlogicutils.h"
//#include "common/tableutils/tablestyleutils.h"
#include "model/cutting/plan/request.h"
#include "model/material/materialmaster.h"

#include <QTableWidgetItem>
//#include <view/managers/stocktablemanager.h>

namespace InputTable{
namespace RowStyler{

inline void applyStyle(QTableWidget* table, int row,
                                const MaterialMaster* mat,
                                const Cutting::Plan::Request& request) {
    if (!mat) return;

    QColor bg = GroupUtils::colorForGroup(mat->id);
    QString groupName = GroupUtils::groupName(mat->id);

    QString baseTip = QString("Anyag: %1\nCsoport: %2\nBarcode: %3")
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
        item->setForeground(bg.lightness() < 128 ? Qt::white : Qt::black);
        item->setTextAlignment(Qt::AlignCenter);

        switch (col) {
        case 0:
            item->setToolTip(baseTip);
            break;
        case 1:
            item->setToolTip(QString("Vágáshossz: %1 mm\n%2").arg(request.requiredLength).arg(baseTip));
            break;
        case 2:
            item->setToolTip(QString("Darabszám: %1 db\n%2").arg(request.quantity).arg(baseTip));
            break;
        default:
            break;
        }
    }
}

} // endof namespace RowStyler
} // endof namespace InputTable
