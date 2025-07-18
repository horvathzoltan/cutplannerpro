#pragma once

 #include "model/materialmaster.h"
// #include <QColor>
#include <QString>

namespace CategoryUtils {

static inline QString categoryToString(ProfileCategory cat) {
    if (cat == ProfileCategory::RollerTube) return "RollerTube";
    if (cat == ProfileCategory::BottomBar)  return "BottomBar";
    return "Unknown";
}

static inline ProfileCategory categoryFromString(const QString& str) {
    if (str == "RollerTube") return ProfileCategory::RollerTube;
    if (str == "BottomBar")  return ProfileCategory::BottomBar;
    return ProfileCategory::Unknown;
}

static inline QString categoryToColorName(ProfileCategory cat) {
    if (cat == ProfileCategory::RollerTube) return "#345678"; // acélkék
    if (cat == ProfileCategory::BottomBar)  return "#506070"; // kékesszürke
    return "#FA8072"; // 😣 lazacszín - hibás / ismeretlen kategória
}

}
