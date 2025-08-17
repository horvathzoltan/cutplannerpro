#pragma once

#include "model/cutting/result/leftoversource.h"
#include <QString>

namespace LeftoverSourceUtils {
QString toString(Cutting::Result::LeftoverSource source) {
    switch (source) {
    case Cutting::Result::LeftoverSource::Manual: return "Manual";
    case Cutting::Result::LeftoverSource::Optimization: return "Optimization";
    default: return "Undefined";
    }
}

Cutting::Result::LeftoverSource fromString(const QString& str) {
    if (str == "Manual") return Cutting::Result::LeftoverSource::Manual;
    if (str == "Optimization") return Cutting::Result::LeftoverSource::Optimization;
    return Cutting::Result::LeftoverSource::Undefined;
}
}
