#pragma once
#include <QTableWidget>
#include <QColor>
#include <QDateTime>
#include "colorlogicutils.h"

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

inline void applyAgeStyle(QTableWidget* table,
                          int row,
                          int colLastSeenAt,
                          const QDateTime& lastSeenAt)
{
    QColor ageColor = ColorLogicUtils::colorForAge(lastSeenAt);

    if (auto* item = table->item(row, colLastSeenAt)) {
        item->setBackground(ageColor);
        item->setForeground(Qt::black);
    }
}

} // namespace LeftoverStyleUtils
