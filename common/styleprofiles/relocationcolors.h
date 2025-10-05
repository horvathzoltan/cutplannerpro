#pragma once
#include <QColor>

namespace RelocationColors {

// 🎨 Globális színprofil – relokációs státuszokhoz

inline const QColor Covered     = QColor(200, 230, 201); // #c8e6c9  ✅ teljes lefedettség
inline const QColor Uncovered   = QColor(255, 205, 210); // #ffcdd2  ❌ hiány
inline const QColor NotAudited  = QColor(255, 224, 178); // #ffe0b2  ⚠️ nem auditált készlet
inline const QColor SummaryBg   = QColor(176, 176, 176); // #b0b0b0  Σ összesítő sor háttér
inline const QColor DefaultFg   = QColor(Qt::black);

}
