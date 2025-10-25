#pragma once

#include <QColor>
#include "materialmaster.h"
#include "materialgroup_utils.h"

namespace MaterialUtils {

static inline QColor colorForMaterial(const MaterialMaster& mat) {
    auto colorName = GroupUtils::colorForGroup(mat.id);
    return QColor(colorName);
}

static inline QString badgeStyleSheet(const MaterialMaster& mat) {
    QColor base = colorForMaterial(mat);
    QColor border = base.darker(130);
    QColor text = QColor("#FFFFFF");

    return QString(
               "background-color: %1;"
               "border: 1px solid %2;"
               "color: %3;"
               "font-weight: bold;"
               "padding: 4px;"
               ).arg(base.name(), border.name(), text.name());
}

static inline QString generateBadgeLabel(const MaterialMaster& mat) {
    return QString("%1 (%2)").arg(mat.name, GroupUtils::groupName(mat.id));
}

static inline QString tooltipForMaterial(const MaterialMaster& mat) {
    return QString("Anyag: %1\nCsoport: %2")
    .arg(mat.name, GroupUtils::groupName(mat.id));
}

static inline QString formatShapeText(const MaterialMaster& mat) {
    const auto shapeEnum = mat.shape.value;

    if (shapeEnum == CrossSectionShape::Shape::Round)
        return QString("Ø%1 mm").arg(mat.diameter_mm);

    if (shapeEnum == CrossSectionShape::Shape::Rectangular)
        return QString("%1×%2 mm").arg(mat.size_mm.width()).arg(mat.size_mm.height());

    return "(ismeretlen forma)";
}
}
