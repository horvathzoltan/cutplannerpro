#pragma once

#include "model/cutresult.h"
#include <QString>

namespace LeftoverSourceUtils {
QString toString(LeftoverSource source) {
    switch (source) {
    case LeftoverSource::Manual: return "Manual";
    case LeftoverSource::Optimization: return "Optimization";
    default: return "Undefined";
    }
}

LeftoverSource fromString(const QString& str) {
    if (str == "Manual") return LeftoverSource::Manual;
    if (str == "Optimization") return LeftoverSource::Optimization;
    return LeftoverSource::Undefined;
}
}
