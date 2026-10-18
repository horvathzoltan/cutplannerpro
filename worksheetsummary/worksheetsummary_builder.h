#pragma once

#include "worksheetsummary/model/worksheetsummary.h"

#include <optional>

class WorkSheetSummaryBuilder
{
public:

    static std::optional<WorkSheetSummary>
    build(
        const QString& fileName);

    static QByteArray
    calculateFileHash(
        const QString& fileName);

};