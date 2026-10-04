#pragma once

#include <QColor>
#include <QString>
#include <QTableWidget>

#include "common/audit/agestate.h"

namespace AgeStyleUtils
{

inline QColor color(AgeState s)
{
    switch (s)
    {
    case AgeState::Missing:
        return QColor(255, 80, 80);

    case AgeState::Old:
        return QColor(255, 160, 120);

    case AgeState::Stale:
        return QColor(255, 240, 170);

    case AgeState::Fresh:
        return QColor(170, 255, 170);

    case AgeState::ExtraFresh:
        return QColor(200, 255, 200);
    }

    return Qt::white;
}

inline QString tooltip(AgeState s)
{
    switch (s)
    {
    case AgeState::Missing:
        return "❌ Hiányzó\nA hulló audit során nem volt megtalálható.";

    case AgeState::Old:
        return "🔴 Régi\nTöbb napja nem volt ellenőrizve.";

    case AgeState::Stale:
        return "🟡 Tegnapi\nAz előző napon volt utoljára ellenőrizve.";

    case AgeState::Fresh:
        return "🟢 Friss\nMa már ellenőrizve volt.";

    case AgeState::ExtraFresh:
        return "✨ Extra friss\nAz elmúlt egy órában volt ellenőrizve.";
    }

    return "Ismeretlen állapot";
}

}

namespace AgeTableUtils
{

inline void applyAgeStyle(
    QTableWidget* table,
    int row,
    int col,
    AgeState state)
{
    if (auto* item = table->item(row, col))
    {
        item->setBackground(
            AgeStyleUtils::color(state));

        item->setForeground(Qt::black);

        item->setToolTip(
            AgeStyleUtils::tooltip(state));
    }
}

}