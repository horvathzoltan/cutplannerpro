#pragma once

#include "leftover/view/dialog/bundlesplitdialog.h"
#include "view/MainWindow.h"
#include "leftover/view/dialog/addwastedialog.h"
#include "stock/view/dialog/editstoragedialog.h"
#include "leftover/registry/leftoverstockregistry.h"
#include "presenter/CuttingPresenter.h"
#include "common/eventlogger.h"

#include <leftover/services/bundlesplitengine.h>

namespace LeftoverTableConnector {
inline static void Connect(
    MainWindow* w,
    LeftoverTableManager* manager,
    LeftoverPresenter* presenter)
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

    // 🔪 Hulló bontása (split)
    w->connect(manager,
               &LeftoverTableManager::splitRequested,
               w,
               [w, presenter](const QUuid& id) {

                   auto opt = LeftoverStockRegistry::instance().findById(id);
                   if (!opt) return;

                   BundleSplitDialog dlg(w);
                   dlg.setModel(*opt);

                   if (dlg.exec() != QDialog::Accepted)
                       return;

                   BundleSplitDialogResult dialogRes = dlg.getResult();

                   zEvent(QString("🔪 Split accepted | leftover=%1 | removed=%2 | remaining=%3")
                              .arg(id.toString())
                              .arg(dialogRes.removedComponents.size())
                              .arg(dialogRes.newComponents.size()));

                   BundleSplitResult engineRes =
                       BundleSplitEngine::applySplit(*opt, dialogRes.newComponents, dialogRes.removedComponents);

                   //presenter->applyBundleSplit(engineRes);
                   // IDE JÖN MAJD A TÉNYLEGES BONTÁSI LOGIKA

                   presenter->applyBundleSplit(engineRes);
                   //presenter->update_LeftoverStockEntry(*opt);

               });


}
}; // end namespace LeftoverTableConnector



