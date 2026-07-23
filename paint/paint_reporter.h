#pragma once

#include "paint/paint_plan.h"

class PaintReporter {
public:
    static QString toText(const PaintPlan& plan);
    static void exportText(const QString& txt);
};
