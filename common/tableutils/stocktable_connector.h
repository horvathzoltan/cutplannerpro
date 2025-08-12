#pragma once

#include "view/MainWindow.h"
#include "view/dialog/stock/addstockdialog.h"
#include "view/dialog/stock/addstockdialog.h"
#include "view/dialog/stock/editstoragedialog.h"
#include "view/dialog/stock/editquantitydialog.h"
#include "view/dialog/stock/editcommentdialog.h"
#include <model/registries/stockregistry.h>
#include <model/registries/cuttingplanrequestregistry.h>
#include <model/registries/leftoverstockregistry.h>
#include <view/dialog/addinputdialog.h>
#include <presenter/CuttingPresenter.h>

namespace StockTableConnector {
inline static void Connect(
    MainWindow *w,
    StockTableManager* manager,
    CuttingPresenter* presenter)
{
    // Törlés
    w->connect(manager, &StockTableManager::deleteRequested, w,
               [presenter](const QUuid& id) {
                   presenter->remove_StockEntry(id);
               });

    // Teljes sor szerkesztése (AddStockDialog)
    w->connect(manager, &StockTableManager::editRequested, w,
               [w, presenter](const QUuid& id) {
                   auto opt = StockRegistry::instance().findById(id);
                   if (!opt) return;

                   StockEntry original = *opt;

                   AddStockDialog dialog(w);
                   dialog.setModel(original);
                   if (dialog.exec() != QDialog::Accepted) return;

                   StockEntry updated = dialog.getModel();
                   presenter->update_StockEntry(updated);
               });

    // Csak mennyiség szerkesztése
    w->connect(manager, &StockTableManager::editQtyRequested, w,
               [w, presenter](const QUuid& id) {
                   auto opt = StockRegistry::instance().findById(id);
                   if (!opt) return;

                   StockEntry original = *opt;

                   EditQuantityDialog dlg(w);
                   dlg.setData(original.quantity);
                   if (dlg.exec() != QDialog::Accepted) return;

                   int newQty = dlg.getData();
                   if (newQty < 0) return; // opcionális validáció

                   original.quantity = newQty;
                   presenter->update_StockEntry(original);
               });

    // Csak tároló szerkesztése
    w->connect(manager, &StockTableManager::editStorageRequested, w,
               [w, presenter](const QUuid& id) {
                   auto opt = StockRegistry::instance().findById(id);
                   if (!opt) return;

                   StockEntry original = *opt;

                   EditStorageDialog dlg(w);
                   dlg.setInitialStorageId(original.storageId);
                   if (dlg.exec() != QDialog::Accepted) return;

                   QUuid newStorageId = dlg.selectedStorageId();
                   if (newStorageId.isNull()) return; // opcionális validáció

                   original.storageId = newStorageId;
                   presenter->update_StockEntry(original);
               });

    // Csak komment szerkesztése
    w->connect(manager, &StockTableManager::editCommentRequested, w,
               [w, presenter](const QUuid& id) {
                   auto opt = StockRegistry::instance().findById(id);
                   if (!opt) return;

                   StockEntry original = *opt;

                   EditCommentDialog dlg(w);
                   dlg.setModel(original.comment);
                   if (dlg.exec() != QDialog::Accepted) return;

                   original.comment = dlg.comment();
                   presenter->update_StockEntry(original);
               });
}
} // end namespace StockTableConnector

