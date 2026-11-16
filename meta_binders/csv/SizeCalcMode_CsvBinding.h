#pragma once

#include "calculation/sizecalcmode.h"


struct SizeCalcMode_CsvBinding
{
    using ValueType = SizeCalcMode;

    static QString toCsv(SizeCalcMode v)
    {
        return SizeCalcModeUtils::toString(v);
    }

    static SizeCalcMode fromCsv( const QString& s)
    {
        return SizeCalcModeUtils::parseSizeCalcMode(s);
    }
};