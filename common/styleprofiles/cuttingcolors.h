#pragma once
#include <QColor>

namespace CuttingColors {

// 🎨 Globális színprofil – vágási státuszokhoz

inline const QColor Pending     = QColor(200, 200, 200); // szürke – még nem futott
inline const QColor InProgress  = QColor(100, 149, 237); // cornflowerblue – folyamatban
inline const QColor Done        = QColor(200, 230, 201); // #c8e6c9 – zöldes, sikeres
inline const QColor Error       = QColor(255, 205, 210); // #ffcdd2 – pirosas, hiba

// 🎨 Szeparátor sor háttér (gépekhez)
inline const QColor MachineSeparatorBg = QColor(176, 196, 222); // lightsteelblue
inline const QColor DefaultFg          = QColor(Qt::black);

}
