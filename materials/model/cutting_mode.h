#pragma once
#include <QString>

enum class CuttingMode {
    None,       // 🚫 Nem vágható
    Length,     // 📏 Szálhossz alapú vágás
    Piece       // 🔩 Darabszám alapú kezelés
};

namespace CuttingModeUtils{
// 🔧 String konverziók
inline QString toString(CuttingMode mode) {
    switch (mode) {
    case CuttingMode::None:   return "None";
    case CuttingMode::Length: return "Length";
    case CuttingMode::Piece:  return "Piece";
    }
    return "Unknown";
}

inline CuttingMode parse(const QString& str) {
    if (str.compare("None", Qt::CaseInsensitive) == 0)   return CuttingMode::None;
    if (str.compare("Length", Qt::CaseInsensitive) == 0) return CuttingMode::Length;
    if (str.compare("Piece", Qt::CaseInsensitive) == 0)  return CuttingMode::Piece;
    return CuttingMode::None; // 🔧 fallback
}

} //end namespace CuttingModeUtils
