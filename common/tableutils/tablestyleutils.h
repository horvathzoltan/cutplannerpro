#pragma once

#include <QTableWidget>

namespace TableStyleUtils{
inline void setCellBackground(QTableWidget* table, int row, int col, const QColor& color) {
    if (auto* widget = table->cellWidget(row, col)) {
        widget->setStyleSheet(QString("background-color: %1").arg(color.name()));
    } else if (auto* item = table->item(row, col)) {
        item->setBackground(color);
    }
}

inline void setCellForeground(QTableWidget* table, int row, int col, const QColor& textColor) {
    if (auto* item = table->item(row, col))
        item->setForeground(textColor);
    else if (auto* widget = table->cellWidget(row, col))
        widget->setStyleSheet(QString("color: %1;").arg(textColor.name()));
}

inline void setCellStyle(QTableWidget* table, int row, int col, const QColor& bgColor, const QColor& fgColor) {
    if (auto* widget = table->cellWidget(row, col)) {
        widget->setStyleSheet(QString("background-color: %1; color: %2;")
                                  .arg(bgColor.name())
                                  .arg(fgColor.name()));
    } else if (auto* item = table->item(row, col)) {
        item->setBackground(bgColor);
        item->setForeground(fgColor);
    }
}
}
