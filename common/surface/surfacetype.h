#pragma once
#include <QString>

enum class SurfaceType {
    Smooth,
    FineStructure,
    CoarseStructure,
    Matt,
    Glossy,
    Satin
};

namespace SurfaceTypeUtils {

inline QString toString(SurfaceType s) {
    switch (s) {
    case SurfaceType::Smooth:        return "Smooth";
    case SurfaceType::FineStructure: return "Fine Structure";
    case SurfaceType::CoarseStructure:return "Coarse Structure";
    case SurfaceType::Matt:          return "Matt";
    case SurfaceType::Glossy:        return "Glossy";
    case SurfaceType::Satin:         return "Satin";
    }
    return "Unknown";
}

inline SurfaceType fromString(const QString& s) {
    QString t = s.trimmed().toLower();
    if (t == "smooth") return SurfaceType::Smooth;
    if (t == "fine structure") return SurfaceType::FineStructure;
    if (t == "coarse structure") return SurfaceType::CoarseStructure;
    if (t == "matt") return SurfaceType::Matt;
    if (t == "glossy") return SurfaceType::Glossy;
    if (t == "satin") return SurfaceType::Satin;
    return SurfaceType::Smooth; // default
}

inline QString toCode(SurfaceType s) {
    switch (s) {
    case SurfaceType::Smooth:        return "SM";
    case SurfaceType::FineStructure: return "FS";
    case SurfaceType::CoarseStructure:return "CS";
    case SurfaceType::Matt:          return "MT";
    case SurfaceType::Glossy:        return "GL";
    case SurfaceType::Satin:         return "ST";
    }
    return "SM";
}

inline SurfaceType fromCode(const QString& code) {
    QString c = code.trimmed().toUpper();
    if (c == "SM") return SurfaceType::Smooth;
    if (c == "FS") return SurfaceType::FineStructure;
    if (c == "CS") return SurfaceType::CoarseStructure;
    if (c == "MT") return SurfaceType::Matt;
    if (c == "GL") return SurfaceType::Glossy;
    if (c == "ST") return SurfaceType::Satin;
    return SurfaceType::Smooth;
}
}
