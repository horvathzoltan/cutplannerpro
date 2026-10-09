#pragma once
#include <QString>

enum class SizeCalcMode {
    Unknown,     // nincs konfiguráció, hiba, hiányzó MSFF
    Gyartasi,
    Uveg,
    Falc,
    Vaszon,
    Perem,
    Tuske
};

namespace SizeCalcModeUtils{
inline QString toString(SizeCalcMode m) {
    switch (m) {
    case SizeCalcMode::Unknown:  return "Unknown";
    case SizeCalcMode::Gyartasi: return "Gyartasi";
    case SizeCalcMode::Uveg:     return "Uveg";
    case SizeCalcMode::Vaszon:   return "Vaszon";
    case SizeCalcMode::Falc:     return "Falc";
    case SizeCalcMode::Perem:    return "Perem";
    case SizeCalcMode::Tuske:    return "Tuske";
    }

    return "Unknown";
}

inline SizeCalcMode parseSizeCalcMode(const QString& s) {
    QString t = s.trimmed().toLower();

    if (t == "gyartasi") return SizeCalcMode::Gyartasi;
    if (t == "uveg")     return SizeCalcMode::Uveg;
    if (t == "vaszon")   return SizeCalcMode::Vaszon;
    if (t == "falc")     return SizeCalcMode::Falc;
    if (t == "perem")    return SizeCalcMode::Perem;
    if (t == "tuske")    return SizeCalcMode::Tuske;

    return SizeCalcMode::Unknown;
}

} // end namespace SizeCalcModeUtils
