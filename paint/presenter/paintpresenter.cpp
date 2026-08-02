#include "paintpresenter.h"
#include "paint/paint_plan.h"

#include <paint/paint_calculator.h>
#include <paint/paint_reporter.h>

PaintPresenter::PaintPresenter(MainWindow* view, QObject *parent)
    : QObject(parent), view(view) {}

void PaintPresenter::ExportPaintPlan(){
    PaintPlan plan = PaintCalculator::buildPlan();
    QString txt = PaintReporter::toText(plan);
    PaintReporter::exportText(txt);
}
