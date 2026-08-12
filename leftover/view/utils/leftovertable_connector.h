#pragma once

#include "view/MainWindow.h"
#include "leftover/view/dialog/addwastedialog.h"
#include "stock/view/dialog/editstoragedialog.h"
#include "leftover/registry/leftoverstockregistry.h"
#include "presenter/CuttingPresenter.h"
#include "common/eventlogger.h"

namespace LeftoverTableConnector {
inline static void Connect(
    MainWindow* w,
    LeftoverTableManager* manager,
    CuttingPresenter* presenter)
{
    // 🗑️ Hulló anyagok törlése
    w->connect(manager,
               &LeftoverTableManager::deleteRequested,
               w,
               [presenter](const QUuid& id) {
                   bool status = presenter->remove_LeftoverStockEntry(id);
                   QString statusTxt = status?"Sikeres":"Sikertelen";

                   zEvent(QString("Selejtezés | %1 | entryId = %2")
                             .arg(statusTxt)
                             .arg(id.toString(QUuid::WithoutBraces)));
               });

    // 📝 Hulló anyagok szerkesztése
    w->connect(manager,
               &LeftoverTableManager::editRequested,
               w,
               [w, presenter](const QUuid& id) {
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

    // 🏷️ Csak tároló szerkesztése (EditStorageDialog)
    w->connect(manager, &LeftoverTableManager::editStorageRequested, w,
               [w, presenter](const QUuid& id) {
                   auto opt = LeftoverStockRegistry::instance().findById(id);
                   if (!opt) return;

                   LeftoverStockEntry original = *opt;

                   EditStorageDialog dlg(w);
                   dlg.setInitialStorageId(original.storageId);
                   if (dlg.exec() != QDialog::Accepted) return;

                   QUuid newStorageId = dlg.selectedStorageId();
                   if (newStorageId.isNull()) return;

                   original.storageId = newStorageId;
                   presenter->update_LeftoverStockEntry(original);
    });
}
}; // end namespace LeftoverTableConnector



