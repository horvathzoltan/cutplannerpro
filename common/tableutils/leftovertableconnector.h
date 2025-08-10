#pragma once

#include "view/MainWindow.h"
#include "view/dialog/addwastedialog.h"
#include <model/registries/stockregistry.h>
#include <model/registries/cuttingplanrequestregistry.h>
#include <model/registries/leftoverstockregistry.h>
#include <view/dialog/addinputdialog.h>
#include <presenter/CuttingPresenter.h>

namespace LeftoverTableConnector{
inline static void Connect(
    MainWindow *w,
    LeftoverTableManager* manager,
    CuttingPresenter* presenter)
{
    // 🗑️ Hulló anyagok táblázat kezelése
    w->connect(manager,
               &LeftoverTableManager::deleteRequested,
               w,
               [presenter](const QUuid& id) {
                   presenter->remove_LeftoverStockEntry(id);
               });

    // 📝 Hulló anyagok szerkesztése
    w->connect(manager,
               &LeftoverTableManager::editRequested,
               w,
               [w,presenter](const QUuid& id) {
                   auto opt = LeftoverStockRegistry::instance().findById(id);
                   if (!opt) return;

                   LeftoverStockEntry original = *opt;

                   AddWasteDialog dialog(w);
                   dialog.setModel(original);

                   if (dialog.exec() != QDialog::Accepted)
                       return;

                   LeftoverStockEntry updated = dialog.getModel();
                   presenter->update_LeftoverStockEntry(updated);
               });

}
}; // end namespace LeftoverTableConnector


