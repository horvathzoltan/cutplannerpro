#pragma once

#include <QString>


enum class PaintGeometryType {
    Linear,      // szálanyag (tok, tokfedél, láb, záró)
    Rectangle,   // pofa (dimA × dimB)
    Diameter,    // csavar (dimA = átmérő)
    Custom       // későbbi speciális esetek
};

namespace PaintGeometryTypeUtils{

    inline QString toString(PaintGeometryType t)
    {
        switch (t) {
        case PaintGeometryType::Linear:    return "Linear";
        case PaintGeometryType::Rectangle: return "Rectangle";
        case PaintGeometryType::Diameter:  return "Diameter";
        case PaintGeometryType::Custom:    return "Custom";
        }
        return "Custom"; // fallback
    }

    inline PaintGeometryType parse(const QString& s)
    {
        const QString v = s.trimmed().toLower();

        if (v == "linear")    return PaintGeometryType::Linear;
        if (v == "rectangle") return PaintGeometryType::Rectangle;
        if (v == "diameter")  return PaintGeometryType::Diameter;
        if (v == "custom")    return PaintGeometryType::Custom;

        // fallback: ha hibás vagy ismeretlen
        return PaintGeometryType::Custom;
    }


}