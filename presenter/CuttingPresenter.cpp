#include "CuttingPresenter.h"
#include "../view/MainWindow.h"
#include "common/cutresultutils.h"

#include <model/registries/stockregistry.h>


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

void CuttingPresenter::setReusableInventory(const QVector<ReusableStockEntry>& list) {
    model.setReusableInventory(list);
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
        // ez a közéspső - eredmény tábla
        view->update_ResultsTable(model.getPlans());
        // ez a készlet
        view->update_stockTable(); // ha a készlet változik
        // ez a maradék

        QVector<CutResult> l = model.getLeftoverResults();
        QVector<ReusableStockEntry> e = CutResultUtils::toReusableEntries(l);
        view->update_leftoversTable(e);

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










