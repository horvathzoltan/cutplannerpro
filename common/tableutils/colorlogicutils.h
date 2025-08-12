#pragma once
#include "common/tableutils/colorconstants.h"
#include <QColor>
#include <QWidget>

namespace ColorLogicUtils {

// 🔵 Hossz alapján háttérszín
inline QColor colorForLength(int length_mm) {
    if (length_mm < 6000)
        return ColorConstants::ColorYellow; // sárgás
    else if (length_mm == 6000)
        return ColorConstants::ColorGreenStandard; // zöld
    else
        return ColorConstants::ColorGreenSuper; // szuperzöld
}

// 🔴 Mennyiség alapján háttérszín
inline QColor colorForQuantity(int quantity) {
    if (quantity == 0)
        return ColorConstants::ColorRed; // piros
    else if (quantity <= 5)
        return ColorConstants::ColorOrange; // narancssárga
    else
        return ColorConstants::ColorGreenStandard; // zöld
}

inline void applyBadgeBackground(QWidget* widget, const QColor& base) {
    widget->setAutoFillBackground(true);
    widget->setStyleSheet(QString(
                              "background-color: %1;"
                              "padding-top: 6px; padding-bottom: 6px;"
                              ).arg(base.name()));
}

} // endof namespace ColorLogicUtils
