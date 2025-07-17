#ifndef COMMON_H
#define COMMON_H

#include <QString>


enum class ProfileCategory {
    RollerTube,  // Tengelyek (csőmotorhoz)
    BottomBar,   // Súlyprofilok (redőny, napháló alja)
    Unknown
};

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

static inline QString badgeColorForCategory(ProfileCategory cat) {
    if (cat == ProfileCategory::RollerTube) return "#345678"; // acélkék
    if (cat == ProfileCategory::BottomBar)  return "#506070"; // kékesszürke
    return "#7f8c8d"; // tompaszürke (ismeretlen kategória)
}

}

#endif // COMMON_H
