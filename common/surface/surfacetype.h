#pragma once
#include <QMap>
#include <QString>

enum class SurfaceType {
    Smooth,
    FineStructure,
    CoarseStructure,
    Matt,
    Glossy,
    Satin,
    Unknown
};

namespace SurfaceTypeUtils {

inline QString toDisplayText(SurfaceType s) {
    switch (s) {
    case SurfaceType::Smooth:        return "Smooth";
    case SurfaceType::FineStructure: return "Fine Structure";
    case SurfaceType::CoarseStructure:return "Coarse Structure";
    case SurfaceType::Matt:          return "Matt";
    case SurfaceType::Glossy:        return "Glossy";
    case SurfaceType::Satin:         return "Satin";
    default: return "Unknown";
    }
}

// inline SurfaceType fromString(const QString& s) {
//     QString t = s.trimmed().toLower();
//     if (t == "smooth") return SurfaceType::Smooth;
//     if (t == "fine structure") return SurfaceType::FineStructure;
//     if (t == "coarse structure") return SurfaceType::CoarseStructure;
//     if (t == "matt") return SurfaceType::Matt;
//     if (t == "glossy") return SurfaceType::Glossy;
//     if (t == "satin") return SurfaceType::Satin;
//     return SurfaceType::Smooth; // default
// }


inline QString toCode(SurfaceType s) {
    switch (s) {
    case SurfaceType::Smooth:        return "SM";
    case SurfaceType::FineStructure: return "FS";
    case SurfaceType::CoarseStructure:return "CS";
    case SurfaceType::Matt:          return "MT";
    case SurfaceType::Glossy:        return "GL";
    case SurfaceType::Satin:         return "ST";
    default: return "Unknown";
    }
}

inline SurfaceType fromCode(const QString& code) {
    QString c = code.trimmed().toUpper();
    if (c == "SM") return SurfaceType::Smooth;
    if (c == "FS") return SurfaceType::FineStructure;
    if (c == "CS") return SurfaceType::CoarseStructure;
    if (c == "MT") return SurfaceType::Matt;
    if (c == "GL") return SurfaceType::Glossy;
    if (c == "ST") return SurfaceType::Satin;
    return SurfaceType::Unknown;
}

inline const QMap<QString, QString>& normalizeMap() {
    static const QMap<QString, QString> table = {
        {"FSTR", "FS"},
        {"FST",  "FS"},
        {"FSL",  "FS"},
        {"FSLR", "FS"},
        {"FSLB", "FS"},
        {"FSLW", "FS"},

        {"SM", "SM"},
        {"CS", "CS"},
        {"MT", "MT"},
        {"GL", "GL"},
        {"ST", "ST"}
    };
    return table;
}

inline QString normalizeRaw(const QString& raw) {
    QString key = raw.trimmed().toUpper();
    if (normalizeMap().contains(key))
        return normalizeMap().value(key);
    return key;
}

inline bool isSurfaceCode(const QString& raw) {
    QString key = raw.trimmed().toUpper();
    return normalizeMap().contains(key);
}

inline SurfaceType fromRawCode(const QString& raw) {
    QString normalized = normalizeRaw(raw);   // FS, SM, CS, MT, GL, ST
    return fromCode(normalized);
}

inline std::pair<QString, SurfaceType> extractSurface(const QString& raw) {
    QStringList parts = raw.trimmed().split(' ', Qt::SkipEmptyParts);
    if (parts.isEmpty())
        return {"", SurfaceType::Unknown};

    QString last = parts.last().trimmed().toUpper();

    if (isSurfaceCode(last)) {
        parts.removeLast();
        QString color = parts.join(" ");
        SurfaceType st = fromRawCode(last);
        return {color, st};
    }

    return {raw.trimmed(), SurfaceType::Unknown};
}

}
