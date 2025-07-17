#include "CuttingPresenter.h"
#include "../view/MainWindow.h"

#include <model/stockrepository.h>


CuttingPresenter::CuttingPresenter(MainWindow* view, QObject *parent)
    : QObject(parent), view(view) {}

void CuttingPresenter::addCutRequest(const CuttingRequest& req) {
    model.addRequest(req);
}

void CuttingPresenter::setCuttingRequests(const QVector<CuttingRequest>& list) {
    model.setRequests(list);
}

void CuttingPresenter::setStockInventory(const QVector<StockEntry>& list) {
    model.setStockInventory(list);
}

void CuttingPresenter::setKerf(int kerf) {
    model.setKerf(kerf);
}

void CuttingPresenter::clearRequests() {
    model.clearRequests();
}

void CuttingPresenter::runOptimization() {
    model.optimize();

    // ✨ Ha készen állsz rá, itt frissíthetjük a View táblákat:
    if (view) {
        view->updatePlanTable(model.getPlans());
        view->appendLeftovers(model.getLeftoverResults());
        view->updateStockTableFromRegistry(); // ha a készlet változik
    }
}

QVector<CutPlan> CuttingPresenter::getPlans()
{
    return model.getPlans();
}

QVector<CutResult> CuttingPresenter::getLeftoverResults()
{
    return model.getLeftoverResults();
}










