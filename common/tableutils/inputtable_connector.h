#pragma once

#include "view/MainWindow.h"
#include <model/registries/stockregistry.h>
#include <model/registries/cuttingplanrequestregistry.h>
#include <model/registries/leftoverstockregistry.h>
#include <view/dialog/addinputdialog.h>
#include <presenter/CuttingPresenter.h>

namespace InputTableConnector{
inline static void Connect(
    MainWindow *w,
    InputTableManager* manager,
    CuttingPresenter* presenter)
{
    w->connect(
        manager,
        &InputTableManager::deleteRequested,
        w,
        [presenter](const QUuid& id) {
            presenter->remove_CuttingPlanRequest(id);
        });

    //
    w->connect(
        manager,
        &InputTableManager::editRequested,
        w,
        [w, presenter](const QUuid& id) {
            auto opt = CuttingPlanRequestRegistry::instance().findById(id);
            if (!opt) return;

            Cutting::Plan::Request original = *opt;

            AddInputDialog dialog(w);
            dialog.setModel(original);

            if (dialog.exec() != QDialog::Accepted)
                return;

            Cutting::Plan::Request updated = dialog.getModel();
            presenter->update_CuttingPlanRequest(updated);
        });
}
}; //end namespace InputTableConnector
