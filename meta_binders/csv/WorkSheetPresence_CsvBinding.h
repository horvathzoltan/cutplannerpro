#pragma once

#include "worksheetsummary/model/worksheetsummary.h"


struct WorkSheetPresence_CsvBinding
{
    using ValueType = WorkSheetPresence;

    static QString toCsv(WorkSheetPresence v)
    {
        return WorkSheetPresenceUtils::toCsv(v);
    }

    static WorkSheetPresence fromCsv(const QString& s)
    {
        return WorkSheetPresenceUtils::fromCsv(s);
    }
};
