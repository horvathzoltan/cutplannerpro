#pragma once

#include <QString>

enum class AgeState
{
    ExtraFresh,   // 1 órán belül
    Fresh,        // aznapi
    Stale,        // tegnapi
    Old,          // több napos
    Missing       // audit szerint nem található
};

namespace AgeStateUtils
{

inline QString toString(AgeState s)
{
    switch (s)
    {
    case AgeState::ExtraFresh: return "ExtraFresh";
    case AgeState::Fresh:      return "Fresh";
    case AgeState::Stale:      return "Stale";
    case AgeState::Old:        return "Old";
    case AgeState::Missing:    return "Missing";
    }

    return "Unknown";
}

inline QString toDisplayString(AgeState s)
{
    switch (s)
    {
    case AgeState::ExtraFresh: return "Extra friss";
    case AgeState::Fresh:      return "Friss";
    case AgeState::Stale:      return "Tegnapi";
    case AgeState::Old:        return "Régi";
    case AgeState::Missing:    return "Hiányzik";
    }

    return "Ismeretlen";
}

inline QString toShortCode(AgeState s)
{
    switch (s)
    {
    case AgeState::ExtraFresh: return "EF";
    case AgeState::Fresh:      return "F";
    case AgeState::Stale:      return "S";
    case AgeState::Old:        return "O";
    case AgeState::Missing:    return "M";
    }

    return "?";
}

}