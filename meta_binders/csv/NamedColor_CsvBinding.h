#pragma once

#include "common/color/namedcolor.h"

struct NamedColor_CsvBinding
{
    using ValueType = NamedColor;

    static QString toCsv(const NamedColor& c)
    {
        return c.code();
    }

    static NamedColor fromCsv(const QString& s)
    {
        return NamedColor(s);
    }
};