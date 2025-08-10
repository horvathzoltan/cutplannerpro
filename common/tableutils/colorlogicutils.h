#pragma once
#include "common/tableutils/colorconstants.h"
#include <QColor>

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
}
