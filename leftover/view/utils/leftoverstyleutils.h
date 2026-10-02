#pragma once
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

// inline void applyAgeStyle(QTableWidget* table,
//                           int row,
//                           int colLastSeenAt,
//                           const QDateTime& lastSeenAt, int notFoundCount)
// {

//     QColor ageColor = notFoundCount>0
//                           ? QColor(255, 160, 160)
//                           :ColorLogicUtils::colorForAge(lastSeenAt);

//     if (auto* item = table->item(row, colLastSeenAt)) {
//         item->setBackground(ageColor);
//         item->setForeground(Qt::black);
//     }
// }


inline void applyAgeStyle_2(QTableWidget* table,
                          int row,
                          int colLastSeenAt,
                          const LeftoverStockEntry& e)
{
    QColor ageColor;

    if (e.isMissing()) {
        ageColor = QColor(255, 160, 160);        // piros – nincs meg
    }
    else if (e.isExtraFresh()) {
        ageColor = QColor(200, 255, 200);        // halványzöld – extra friss
    }
    else if (e.isFresh()) {
        ageColor = QColor(170, 255, 170);        // zöld – aznapi
    }
    else { // e.isStale()
        ageColor = QColor(255, 240, 170);        // sárga – régi
    }

    if (auto* item = table->item(row, colLastSeenAt)) {
        item->setBackground(ageColor);
        item->setForeground(Qt::black);
    }
}


} // namespace LeftoverStyleUtils
