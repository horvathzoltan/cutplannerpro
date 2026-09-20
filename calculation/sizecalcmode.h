#pragma once
#include <QString>

enum class SizeCalcMode {
    Unknown,     // nincs konfiguráció, hiba, hiányzó MSFF
    Gyartasi,
    Uveg,
    Vaszon
};

namespace SizeCalcModeUtils{
inline QString toString(SizeCalcMode m) {
    switch (m) {
    case SizeCalcMode::Unknown:  return "Unknown";
    case SizeCalcMode::Gyartasi: return "Gyartasi";
    case SizeCalcMode::Uveg:     return "Uveg";
    case SizeCalcMode::Vaszon:   return "Vaszon";
    }

    return "Unknown";
}

inline SizeCalcMode parseSizeCalcMode(const QString& s) {
    QString t = s.trimmed().toLower();

    if (t == "gyartasi") return SizeCalcMode::Gyartasi;
    if (t == "uveg")     return SizeCalcMode::Uveg;
    if (t == "vaszon")   return SizeCalcMode::Vaszon;

    return SizeCalcMode::Unknown;
}

} // end namespace SizeCalcModeUtils