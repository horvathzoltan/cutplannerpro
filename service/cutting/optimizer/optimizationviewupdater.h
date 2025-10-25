#pragma once

//#include "presenter/CuttingPresenter.h"
#include "view/MainWindow.h"
#include "model/cutting/result/resultmodel.h"
#include "model/leftoverstockentry.h"
#include "service/cutting/result/resultutils.h"

struct OptimizationViewUpdater {
    static void update(MainWindow* view, Cutting::Optimizer::OptimizerModel& model) {
        auto& plans = model.getResult_PlansRef();

        // ✨ UI frissítése
        view->update_ResultsTable(plans);
        view->refresh_StockTable();

        // Hullók vizuális frissítése
        QVector<Cutting::Result::ResultModel> leftovers = model.getResults_Leftovers();
        QVector<LeftoverStockEntry> reusable = Cutting::Result::ResultUtils::toReusableEntries(leftovers);
        Q_UNUSED(reusable);

        view->refresh_LeftoversTable();
        view->updateStats(plans, leftovers);
    }
};

