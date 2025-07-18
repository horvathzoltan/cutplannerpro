#pragma once

#include <QColor>
#include "../model/materialmaster.h"
#include "common/categoryutils.h"

namespace MaterialUtils {

static inline QColor colorForMaterial(const MaterialMaster& mat) {
    auto colorName = CategoryUtils::categoryToColorName(mat.category);
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
    return QString("%1 (%2)")
    .arg(mat.name, CategoryUtils::categoryToString(mat.category));
}

static inline QString tooltipForMaterial(const MaterialMaster& mat) {
    return QString("Anyag: %1\nKategória: %2")
        .arg(mat.name, CategoryUtils::categoryToString(mat.category));
}


}
