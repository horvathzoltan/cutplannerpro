#pragma once

#include "model/cutplan.h"
#include <QVector>

namespace OptimizationExporter {
void exportPlansToCSV(const QVector<CutPlan>& plans, const QString& folderPath = {});
void exportPlansAsWorkSheetTXT(const QVector<CutPlan>& plans, const QString& folderPath = {});
}
