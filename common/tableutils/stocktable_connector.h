#pragma once

#include "view/MainWindow.h"
#include "view/dialog/movement/movementdialog.h"
#include "view/dialog/stock/addstockdialog.h"
#include "view/dialog/stock/addstockdialog.h"
#include "view/dialog/stock/editstoragedialog.h"
#include "view/dialog/stock/editquantitydialog.h"
#include "view/dialog/stock/editcommentdialog.h"
#include <model/registries/stockregistry.h>
#include <model/registries/cuttingplanrequestregistry.h>
#include <model/registries/leftoverstockregistry.h>
#include <model/registries/storageregistry.h>
#include <model/registries/materialregistry.h>
#include <view/dialog/addinputdialog.h>
#include <presenter/CuttingPresenter.h>

#include "service/movementlogger.h"   // a namespace-es inline log() miatt

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
                   presenter->update_StockEntry(original); // már benne van az AuditStateManager trigger
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

    // mozgatás
    w->connect(manager, &StockTableManager::moveRequested, w,
               [w, presenter](const QUuid& id) {

                   auto opt = StockRegistry::instance().findById(id);
                   if (!opt) return;
                   const StockEntry& original = *opt;

                   // Forrás raktár információk a UI-hoz és a loghoz
                   const auto* srcStorage = StorageRegistry::instance().findById(original.storageId);
                   const QString srcName = srcStorage ? srcStorage->name : "—";
                   // Ha van barcode meződ a raktárnál:
                   const QString srcBarcode = srcStorage ? srcStorage->barcode : QString();

                   MovementDialog dlg(w);
                   dlg.setSource(srcName, original.entryId, original.quantity);
                   if (dlg.exec() != QDialog::Accepted) return;

                   MovementData data = dlg.getMovementData();
                   // Alap validáció
                   if (data.quantity <= 0) return;
                   if (data.quantity > original.quantity) {
                       // Opcionálisan jelezd a felhasználónak
                       // QMessageBox::warning(w, "Érvénytelen mennyiség", "A kért mennyiség nagyobb, mint a rendelkezésre álló.");
                       return;
                   }
                   if (data.toStorageId.isNull()) return;

                   // Cél raktár információk a loghoz
                   const auto* destStorage = StorageRegistry::instance().findById(data.toStorageId);
                   const QString destName = destStorage ? destStorage->name : "—";
                   const QString destBarcode = destStorage ? destStorage->barcode : QString();

                   // Ha értelmezett, ne engedjük ugyanabba a raktárba mozgatni
                   if (destStorage && srcStorage && destStorage->id == srcStorage->id) {
                       // QMessageBox::information(w, "Nincs művelet", "A forrás és cél raktár azonos.");
                       return;
                   }

                   // Új bejegyzés az áthelyezett mennyiséggel
                   StockEntry movedEntry = original;
                   movedEntry.entryId = QUuid::createUuid();
                   movedEntry.storageId = data.toStorageId;
                   movedEntry.quantity = data.quantity;
                   movedEntry.comment = data.comment;

                   const int remainingQty = original.quantity - data.quantity;

                   // Állapotmódosítás – csak siker esetén logolunk
                   if (remainingQty > 0) {
                       // Részmozgás: előbb csökkentjük az eredetit, majd hozzáadjuk az újat
                       StockEntry updatedOriginal = original;
                       updatedOriginal.quantity = remainingQty;
                       presenter->update_StockEntry(updatedOriginal);
                       presenter->add_StockEntry(movedEntry);
                   } else {
                       // Teljes áthelyezés: új bejegyzés, majd a régi törlése
                       presenter->add_StockEntry(movedEntry);
                       presenter->remove_StockEntry(original.entryId);
                   }

                   // LOG – csak sikeres módosítás után
                   // Gazdagítsuk a MovementData-t névvel/barcode-dal (ha van ilyen mező a struktúrában)
                   // Ha a MovementData-t kibővítetted korábban:
                   data.fromEntryId = original.entryId;
                   // Ha a Storage/Item rendelkezik ezekkel, töltsd:
                   data.fromStorageName = srcName;
                   data.fromBarcode = srcBarcode;
                   data.toStorageName = destName;
                   data.toBarcode = destBarcode;

                   // Ha van termék entitásod:
                   const auto* item = MaterialRegistry::instance().findById(original.materialId);
                   data.itemName = item ? item->name : QString();
                   data.itemBarcode = item ? item->barcode : QString();

                   MovementLogModel logdata{data};
                   MovementLogger::log(logdata);
               });

}
} // end namespace StockTableConnector


