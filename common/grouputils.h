#pragma once

#include <QString>
#include <QColor>
#include <QUuid>
#include <QSet>
#include "../model/registries/materialgroupregistry.h"
#include "../model/materialgroup.h"
#include "common/colorutils.h"

namespace GroupUtils {

static inline const MaterialGroup* groupForMaterial(const QUuid& id) {
    return MaterialGroupRegistry::instance().findByMaterialId(id);
}

static inline QString groupName(const QUuid& id) {
    const auto* g = groupForMaterial(id);
    return g ? g->name : QString("(?)");
}

static inline QColor colorForGroup(const QUuid& id) {
    const auto* g = MaterialGroupRegistry::instance().findByMaterialId(id);
    if (!g || g->colorHex.isEmpty()) return QColor("#999999");

    return ColorUtils::parseColor(g->colorHex, QColor("#999999"));
}

static inline QString labelForGroup(const QUuid& id) {
    const auto* g = groupForMaterial(id);
    return g ? QString("%1 profil").arg(g->name) : QString("Egyedi profil");
}

static inline QSet<QUuid> groupMembers(const QUuid& materialId) {
    const auto* group = MaterialGroupRegistry::instance().findByMaterialId(materialId);
    return group ? QSet<QUuid>(group->materialIds.begin(), group->materialIds.end())
                 : QSet<QUuid>{ materialId };
}

}

// #pragma once

//  #include "model/materialmaster.h"
// // #include <QColor>
// #include <QString>

// namespace CategoryUtils {

// static inline QString categoryToString(ProfileCategory cat) {
//     if (cat == ProfileCategory::RollerTube) return "RollerTube";
//     if (cat == ProfileCategory::BottomBar)  return "BottomBar";
//     return "Unknown";
// }

// static inline ProfileCategory categoryFromString(const QString& str) {
//     if (str == "RollerTube") return ProfileCategory::RollerTube;
//     if (str == "BottomBar")  return ProfileCategory::BottomBar;
//     return ProfileCategory::Unknown;
// }

// static inline QString categoryToColorName(ProfileCategory cat) {
//     if (cat == ProfileCategory::RollerTube) return "#345678"; // acélkék
//     if (cat == ProfileCategory::BottomBar)  return "#506070"; // kékesszürke
//     return "#FA8072"; // 😣 lazacszín - hibás / ismeretlen kategória
// }

// }
