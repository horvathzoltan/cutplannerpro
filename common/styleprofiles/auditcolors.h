#pragma once
#include <QColor>

namespace AuditColors {

// 🎨 Globális színprofil – típusos, konstans értékek
inline const QColor Ok        = QColor(200, 230, 201); // #c8e6c9
inline const QColor Missing   = QColor(255, 205, 210); // #ffcdd2
inline const QColor Pending   = QColor(255, 249, 196); // #fff9c4
inline const QColor Info      = QColor(Qt::lightGray);
inline const QColor Unknown   = QColor(Qt::lightGray);

inline const QColor LeftoverBg = QColor(227, 242, 253); // #e3f2fd
inline const QColor DefaultFg  = QColor(Qt::black);

}
