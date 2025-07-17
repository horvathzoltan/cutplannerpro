#include "CuttingPresenter.h"
#include "../view/MainWindow.h"

#include <model/stockrepository.h>


CuttingPresenter::CuttingPresenter(MainWindow* view, QObject *parent)
    : QObject(parent), view(view) {}

void CuttingPresenter::addRequest(const CuttingRequest& req) {
    model.addRequest(req);
}

void CuttingPresenter::setKerf(int kerf) {
    model.setKerf(kerf);
}

void CuttingPresenter::clearRequests() {
    model.clearRequests();
}

void CuttingPresenter::runOptimization() {
    model.optimize();
    //view->updatePlanTable(model.getPlans());
    //view->updateLeftovers(model.getLeftoverResults());
}

QVector<CutPlan> CuttingPresenter::getPlans()
{
    return model.getPlans();
}

QVector<CutResult> CuttingPresenter::getLeftoverResults()
{
    return model.getLeftoverResults();
}


// void CuttingPresenter::updateLeftovers() {
//     QVector<CutResult> leftovers = model.getLeftoverResults();
//     view->updateLeftoversTable(leftovers);
// }



/**/

void CuttingPresenter::setCuttingRequests(const QVector<CuttingRequest>& list) {
    model.setRequests(list);
}

void CuttingPresenter::setStockInventory(const QVector<StockEntry>& list) {
    model.setStockInventory(list);
}





