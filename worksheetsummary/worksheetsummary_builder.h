#pragma once

#include "model/cutting/plan/request.h"
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

    static QVector<WorkSheetOrderInfo>
    buildOrderInfos(
        const QVector<Cutting::Plan::Request>& requests);
};