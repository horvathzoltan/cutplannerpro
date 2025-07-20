#pragma once
#include <QColor>
#include <QString>

namespace ColorUtils
{

static inline bool validateColorHex(const QString& hex) {
    if (hex.size() != 7 || !hex.startsWith('#')) return false;
    const QString code = hex.mid(1);
    for (const QChar& c : code) {
        if (!c.isDigit() && !(c >= 'A' && c <= 'F') && !(c >= 'a' && c <= 'f'))
            return false;
    }
    return true;
}

inline QColor parseColor(const QString& hex, const QColor& fallback = QColor("#999999")) {
    return validateColorHex(hex) ? QColor(hex) : fallback;
}

}
