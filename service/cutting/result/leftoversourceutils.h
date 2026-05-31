#pragma once

#include "../../../model/cutting/result/leftoversource.h"
#include <QString>

namespace LeftoverSourceUtils {
inline QString toString(Cutting::Result::LeftoverSource source) {
    switch (source) {
    case Cutting::Result::LeftoverSource::Manual: return "Manual";
    case Cutting::Result::LeftoverSource::Optimization: return "Optimization";
    default: return "Undefined";
    }
}

inline Cutting::Result::LeftoverSource fromString(const QString& str) {
    QString k = str.trimmed().toLower();
    if (k == "manual") return Cutting::Result::LeftoverSource::Manual;
    if (k == "optimization") return Cutting::Result::LeftoverSource::Optimization;
    return Cutting::Result::LeftoverSource::Undefined;
}
}
