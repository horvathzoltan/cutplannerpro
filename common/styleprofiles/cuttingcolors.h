#pragma once
#include <QColor>

namespace CuttingColors {

// 🎨 Globális színprofil – vágási státuszokhoz

inline const QColor Pending     = QColor(200, 200, 200); // szürke – még nem futott
inline const QColor InProgress  = QColor(231, 76, 60);   // piros (#e74c3c) – folyamatban
inline const QColor Done        = QColor(46, 204, 113);  // zöld (#2ecc71) – sikeres
inline const QColor Error       = QColor(255, 205, 210); // #ffcdd2 – pirosas, hiba

// 🎨 Szeparátor sor háttér (gépekhez)
inline const QColor MachineSeparatorBg = QColor(176, 196, 222); // lightsteelblue
inline const QColor DefaultFg          = QColor(Qt::black);

}
