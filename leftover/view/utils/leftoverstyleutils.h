#pragma once
#include "common/audit/agestyleutils.h"
#include "leftover/model/leftoverstockentry.h"
#include "view/tableutils/colorlogicutils.h"
#include <QTableWidget>
#include <QColor>
#include <QDateTime>

namespace LeftoverStyleUtils {

inline void applyPrefixStyle(QTableWidget* table,
                             int row,
                             int colBarcode,
                             const QString& barcode)
{
    QString prefix = barcode.left(3).toUpper();
    bool ok = (prefix == "RSM" || prefix == "RST");

    if (!ok) {
        if (auto* item = table->item(row, colBarcode)) {
            item->setBackground(QColor(255, 220, 220));
            item->setForeground(Qt::black);
            item->setToolTip(QString(
                                 "⚠️ Hibás leftover kód: '%1'\n"
                                 "Csak RSM és RST prefix engedélyezett."
                                 ).arg(prefix));
        }
    }
}


// inline void applyAgeStyle_2(QTableWidget* table,
//                             int row,
//                             int colLastSeenAt,
//                             const LeftoverStockEntry& e)
// {
//     //const AgeState state = e.ageState();

//     // if (auto* item = table->item(row, colLastSeenAt))
//     // {
//     //     item->setBackground(AgeStyleUtils::color(state));
//     //     item->setForeground(Qt::black);
//     //     item->setToolTip(AgeStyleUtils::tooltip(state));
//     // }
//     AgeTableUtils::applyAgeStyle(table,row,colLastSeenAt,e.ageState());
// }



} // namespace LeftoverStyleUtils
