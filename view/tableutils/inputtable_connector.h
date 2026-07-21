#pragma once

#include "../MainWindow.h"
#include "../../model/registries/stockregistry.h"
#include "../../model/registries/cuttingplanrequestregistry.h"
#include "../../model/registries/leftoverstockregistry.h"
#include "../dialog/input/addinputdialog.h"
#include "../../presenter/CuttingPresenter.h"

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
        [w, presenter](const QUuid& id) {

            auto opt = CuttingPlanRequestRegistry::instance().findById(id);
            if (opt) {
                auto r = *opt;
            w->seriesMatrixView()->removeFilledCell(r.externalReference, r.materialId);
            }

            presenter->remove_CuttingPlanRequest(id);

            // ⭐ BOM cache kiütése
            w->seriesMatrixView()->clearBomCache();
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

            AddInputDialog dialog(w, DialogMode::Update, &original);
            // QObject::connect(&dialog, &AddInputDialog::seriesContextChanged,
            //         w->seriesMatrixView(), &SeriesMatrixView::onSeriesContextChanged);

            if (dialog.exec() != QDialog::Accepted)
                return;

            zInfo("InputTableConnector getModel start");
            Cutting::Plan::Request updated = dialog.getModel();
            zInfo("InputTableConnector getModel end");
            presenter->update_AllRequestsWithSameReference(updated);
            presenter->update_CuttingPlanRequest(updated);

            // ⭐ filledCells cache frissítése
            w->seriesMatrixView()->updateFilledCell(
                original.externalReference, original.materialId,
                updated.externalReference, updated.materialId
                );

            // ⭐ BOM cache kiütése
            w->seriesMatrixView()->clearBomCache();

            // ⭐ Mátrix frissítése UPDATE után
            w->seriesMatrixView()->refreshAfterAdd(updated.externalReference);
        });
}
}; //end namespace InputTableConnector
