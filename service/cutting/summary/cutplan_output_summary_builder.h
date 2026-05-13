#pragma once
#include "cutplan_output_summary.h"
#include <model/cutting/plan/cutplan.h>
#include <model/cutting/result/resultmodel.h>

struct CutPlanOutputSummaryBuilder {
    CutPlanOutputSummary build(
        const QVector<Cutting::Plan::CutPlan>& plans,
        const QVector<Cutting::Result::ResultModel>& leftovers) const;
};
