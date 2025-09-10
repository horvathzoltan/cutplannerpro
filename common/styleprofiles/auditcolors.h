#pragma once
#include <QColor>

namespace AuditColors {
inline QColor ok()        { return QColor("#c8e6c9"); }
inline QColor missing()   { return QColor("#ffcdd2"); }
inline QColor pending()   { return QColor("#fff9c4"); }
inline QColor leftoverBg(){ return QColor("#e3f2fd"); }
inline QColor defaultFg() { return Qt::black; }
}
