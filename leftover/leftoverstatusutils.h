#pragma once

#include <QString>
#include "leftoverstatus.h"

namespace LeftoverStatusUtils{
inline QString toString(LeftoverStatus st) {
    switch (st) {
    case LeftoverStatus::Unknown:        return "Unknown";
    case LeftoverStatus::Good:           return "Good";
    case LeftoverStatus::SurfaceDamage:  return "SurfaceDamage";
    case LeftoverStatus::Bent:           return "Bent";
    case LeftoverStatus::Dirty:          return "Dirty";
    case LeftoverStatus::Sticky:         return "Sticky";
    case LeftoverStatus::Reserved:       return "Reserved";
    case LeftoverStatus::Scrapped:       return "Scrapped";
    }
    return "Unknown";
}

inline LeftoverStatus fromString(const QString& s0) {
    QString s = s0.trimmed().toLower();
    if (s == "unknown")        return LeftoverStatus::Unknown;
    if (s == "good")           return LeftoverStatus::Good;
    if (s == "surfacedamage")  return LeftoverStatus::SurfaceDamage;
    if (s == "bent")           return LeftoverStatus::Bent;
    if (s == "dirty")          return LeftoverStatus::Dirty;
    if (s == "sticky")         return LeftoverStatus::Sticky;
    if (s == "reserved")       return LeftoverStatus::Reserved;
    if (s == "scrapped")       return LeftoverStatus::Scrapped;

    return LeftoverStatus::Unknown; // fallback
}

} //endof