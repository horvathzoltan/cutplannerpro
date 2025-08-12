#pragma once

#include "model/leftoverstockentry.h"
#include <view/managers/leftovertablemanager.h>
#include "common/materialutils.h"

namespace LeftoverTable{
namespace RowStyler{

inline void applyStyle(QTableWidget* table, int row, const MaterialMaster* master, const LeftoverStockEntry& entry) {
    if (!table || !master)
        return;

    constexpr int ColReusable = LeftoverTableManager::ColReusable; // 🔁 Frissítsd, ha eltér;

    // 🎨 Kategóriaalapú háttérszín
    QColor baseColor = MaterialUtils::colorForMaterial(*master);

    for (int col = 0; col < table->columnCount(); ++col) {
        if (col == ColReusable)
            continue;

        auto* item = table->item(row, col);
        if (!item) {
            item = new QTableWidgetItem();
            table->setItem(row, col, item);
        }

        item->setBackground(baseColor);
        item->setForeground(col == 0 ? Qt::white : Qt::black); // Név oszlop legyen világos betűs
    }

    // ♻️ Újrahasználhatóság cella stílusa
    QString reuseMark = (entry.availableLength_mm >= 300) ? "✔" : "✘";
    auto* itemReuse = table->item(row, ColReusable);
    if (!itemReuse) {
        itemReuse = new QTableWidgetItem(reuseMark);
        table->setItem(row, ColReusable, itemReuse);
    } else {
        itemReuse->setText(reuseMark);
    }

    itemReuse->setTextAlignment(Qt::AlignCenter);
    itemReuse->setBackground(reuseMark == "✔" ? QColor(144, 238, 144) : QColor(255, 200, 200));
    itemReuse->setForeground(Qt::black);
}

} // endof namespace RowStyler
} // endof namespace StockTable
