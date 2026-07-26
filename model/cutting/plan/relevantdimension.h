#pragma once

#include <QString>


enum class RelevantDimension { Width, Height };

namespace RelevantDimensionUtils{
    inline QString toString(RelevantDimension d)
    {
        switch (d) {
        case RelevantDimension::Width:  return "Width";
        case RelevantDimension::Height: return "Height";
        }
        return "Unknown";
    }

    inline RelevantDimension fromString(const QString& s)
    {
        const QString t = s.trimmed().toLower();

        if (t == "width")  return RelevantDimension::Width;
        if (t == "height") return RelevantDimension::Height;

        // Default fallback – Width vagy dobhatnánk is
        return RelevantDimension::Width;
    }

    inline bool tryParse(const QString& s, RelevantDimension& out)
    {
        const QString t = s.trimmed().toLower();

        if (t == "width") {
            out = RelevantDimension::Width;
            return true;
        }
        if (t == "height") {
            out = RelevantDimension::Height;
            return true;
        }
        return false;
    }
};