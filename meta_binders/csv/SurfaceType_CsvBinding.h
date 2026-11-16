#pragma once

#include "common/surface/surfacetype.h"

struct SurfaceType_CsvBinding
{
    using ValueType = SurfaceType;

    static QString toCsv(SurfaceType v)
    {
        return SurfaceTypeUtils::toCode(v);
    }

    static SurfaceType fromCsv( const QString& s)
    {
        return SurfaceTypeUtils::fromCode(s);
    }
};