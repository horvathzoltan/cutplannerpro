#pragma once

#include "model/cutting/plan/cutplan.h"
#include <QVector>

namespace OptimizationExporter {
    void exportPlansToCSV(const QVector<Cutting::Plan::CutPlan>& plans, const QString& folderPath = {});
    void exportPlansAsWorkSheetTXT(const QVector<Cutting::Plan::CutPlan>& plans, const QString& folderPath = {});
    void exportPlans(const QVector<Cutting::Plan::CutPlan>& plans);

    static QString displayText(const Cutting::Piece::PieceWithMaterial& m);
}
