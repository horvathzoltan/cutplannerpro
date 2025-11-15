#pragma once

#include "../MainWindow.h"
#include "../dialog/movement/movementdialog.h"
#include "../dialog/stock/addstockdialog.h"
#include "../dialog/stock/addstockdialog.h"
#include "../dialog/stock/editstoragedialog.h"
#include "../dialog/stock/editquantitydialog.h"
#include "../dialog/stock/editcommentdialog.h"
#include "../../model/registries/stockregistry.h"
#include "../../model/registries/cuttingplanrequestregistry.h"
#include "../../model/registries/leftoverstockregistry.h"
#include "../../model/registries/storageregistry.h"
#include "materials/registry/material_registry.h"
#include "../dialog/input/addinputdialog.h"
#include "../../presenter/CuttingPresenter.h"
#include "../../service/stockmovementservice.h"

//#include "service/movementlogger.h"   // a namespace-es inline log() miatt

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

    //mozgatás
    w->connect(manager, &StockTableManager::moveRequested, w,
               [w, presenter](const QUuid& id) {
                   zInfo(QStringLiteral("➡️ moveRequested: entryId=%1").arg(id.toString()));

                   auto opt = StockRegistry::instance().findById(id);
                   if (!opt) {
                       zWarning(QStringLiteral("⚠️ moveRequested: entryId=%1 nem található a StockRegistry-ben")
                                    .arg(id.toString()));
                       return;
                   }
                   const StockEntry& original = *opt;

                   zInfo(QStringLiteral("📦 Forrás entry: entryId=%1, materialId=%2, storageId=%3, qty=%4")
                             .arg(original.entryId.toString(), original.materialId.toString(), original.storageId.toString())
                             .arg(original.quantity));

                   const auto* srcStorage = StorageRegistry::instance().findById(original.storageId);
                   QString srcLabel = srcStorage
                                          ? QString("%1 (%2)").arg(srcStorage->name, srcStorage->barcode)
                                          : QStringLiteral("—");

                   MovementDialog dlg(w);
                   dlg.setSource(srcLabel, original.entryId, original.quantity);

                   if (dlg.exec() != QDialog::Accepted) {
                       zInfo(QStringLiteral("❌ Mozgatás megszakítva a dialógusban"));
                       return;
                   }

                   MovementData data = dlg.getMovementData();

                   // Biztonsági kitöltések: ha a dialógus nem adta meg, használjuk az eredeti entry és anyag adatait
                   if (data.fromEntryId.isNull()) data.fromEntryId = original.entryId;
                   if (data.materialId.isNull()) data.materialId = original.materialId;

                   // Ellenőrizzük, hogy van-e érvényes cél (toEntryId vagy toStorageId)
                   if (data.toEntryId.isNull() && data.toStorageId.isNull()) {
                       zWarning(QStringLiteral("moveRequested: insufficient movement target information"));
                       return;
                   }

                   // Meghívás és UI visszajelzés
                   if (!StockMovementService::moveStock(data, presenter)) {
                       zWarning(QStringLiteral("⚠️ moveRequested: moveStock failed for fromEntry=%1 qty=%2")
                                    .arg(data.fromEntryId.toString()).arg(data.quantity));
                       return;
                   }
               });

}
} // end namespace StockTableConnector


