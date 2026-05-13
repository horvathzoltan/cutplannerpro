#pragma once

#include "cutplansummary.h"
#include <model/cutting/plan/cutplan.h>
#include <model/cutting/result/resultmodel.h>

class CutPlanSummaryBuilder {
public:
    static CutPlanSummary build(
        const QVector<Cutting::Plan::CutPlan>& plans,
        const QVector<Cutting::Result::ResultModel>& leftovers,
        const QString& planIdStr);
};
