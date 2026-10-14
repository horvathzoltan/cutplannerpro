#pragma once

#include <QString>

namespace Cutting::Plan {

enum class RequestState
{
    Unknown,
    Cut,
    NotCut
};


namespace RequestStateUtils
{
    inline QString toDisplayText(RequestState s)
    {
        switch (s) {
        case RequestState::Unknown: return "Unknown";
        case RequestState::Cut:     return "Cut";
        case RequestState::NotCut:  return "Not cut";
        }

        return "Unknown";
    }

    inline QString toCsv(RequestState s)
    {
        switch (s) {
        case RequestState::Unknown: return "UNKNOWN";
        case RequestState::Cut:     return "CUT";
        case RequestState::NotCut:  return "NOT_CUT";
        }

        return "UNKNOWN";
    }

    inline RequestState fromCsv(const QString& s)
    {
        if (s.compare("UNKNOWN", Qt::CaseInsensitive) == 0)
            return RequestState::Unknown;

        if (s.compare("CUT", Qt::CaseInsensitive) == 0)
            return RequestState::Cut;

        if (s.compare("NOT_CUT", Qt::CaseInsensitive) == 0)
            return RequestState::NotCut;

        return RequestState::Unknown;
    }

    inline bool isKnown(RequestState s)
    {
        return s != RequestState::Unknown;
    }

}

}


