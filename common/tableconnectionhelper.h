#pragma once

#include "view/MainWindow.h"
#include "view/dialog/addstockdialog.h"
#include "view/dialog/addwastedialog.h"
#include <model/registries//stockregistry.h>
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

            CuttingPlanRequest original = *opt;

            AddInputDialog dialog(w);
            dialog.setModel(original);

            if (dialog.exec() != QDialog::Accepted)
                return;

            CuttingPlanRequest updated = dialog.getModel();
            presenter->update_CuttingPlanRequest(updated);
        });
}
}; //end namespace InputTableConnector

namespace StockTableConnector{
inline static void Connect(
    MainWindow *w,
    StockTableManager* manager,
    CuttingPresenter* presenter)
{
    w->connect(manager,
               &StockTableManager::deleteRequested,
               w,
               [presenter](const QUuid& id) {
                   presenter->remove_StockEntry(id);
               });

    w->connect(
        manager,
        &StockTableManager::editRequested,
        w,
        [w,presenter](const QUuid& id) {
            auto opt = StockRegistry::instance().findById(id);
            if (!opt) return;

            StockEntry original = *opt;

            AddStockDialog dialog(w);
            dialog.setModel(original);

            if (dialog.exec() != QDialog::Accepted)
                return;

            StockEntry updated = dialog.getModel();
            presenter->update_StockEntry(updated);
        });

    w->connect(
        manager,
        &StockTableManager::editQtyRequested,
        w,
        [w,presenter](const QUuid& id) {
            auto opt = StockRegistry::instance().findById(id);
            if (!opt) return;

            StockEntry original = *opt;

            AddStockDialog dialog(w);
            dialog.setModel(original);

            if (dialog.exec() != QDialog::Accepted)
                return;

            StockEntry updated = dialog.getModel();
            presenter->update_StockEntry(updated);
        });
}
}; // end namespace StockTableConnector

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

// namespace ButtonConnector{

// inline static void Connect(
//     Ui::MainWindow *ui, MainWindow* w)
// {
//     w->connect(ui->btn_AddCuttingPlanRequest, &QPushButton::clicked,
//             w, &MainWindow::handle_btn_AddCuttingPlanRequest_clicked);
//     w->connect(ui->btn_NewCuttingPlan, &QPushButton::clicked,
//             w, &MainWindow::handle_btn_NewCuttingPlan_clicked);
//     w->connect(ui->btn_ClearCuttingPlan, &QPushButton::clicked,
//             w, &MainWindow::handle_btn_ClearCuttingPlan_clicked);
// }

// template <typename Func1, typename Func2>
// inline static void c2(Func1 signal, Func2 &&slot){

// }
// }
// void MainWindow::Connect_InputTableManager()
// {
//     connect(inputTableManager.get(), &InputTableManager::deleteRequested,
//             this, [this](const QUuid& id) {
//                 presenter->removeCutRequest(id);
//             });

//     //
//     connect(inputTableManager.get(), &InputTableManager::editRequested,
//             this, [this](const QUuid& id) {
//                 auto opt = CuttingPlanRequestRegistry::instance().findById(id);
//                 if (!opt) return;

//                 CuttingPlanRequest original = *opt;

//                 AddInputDialog dialog(this);
//                 dialog.setModel(original);

//                 if (dialog.exec() != QDialog::Accepted)
//                     return;

//                 CuttingPlanRequest updated = dialog.getModel();
//                 presenter->updateCutRequest(updated);
//             });
// }

// void MainWindow::Connect_StockTableManager()
// {
//     connect(stockTableManager.get(), &StockTableManager::deleteRequested,
//             this, [this](const QUuid& id) {
//                 presenter->removeStockEntry(id);
//             });

//     connect(stockTableManager.get(), &StockTableManager::editRequested,
//             this, [this](const QUuid& id) {
//                 auto opt = StockRegistry::instance().findById(id);
//                 if (!opt) return;

//                 StockEntry original = *opt;

//                 AddStockDialog dialog(this);
//                 dialog.setModel(original);

//                 if (dialog.exec() != QDialog::Accepted)
//                     return;

//                 StockEntry updated = dialog.getModel();
//                 presenter->updateStockEntry(updated);
//             });
// }

// void MainWindow::Connect_LeftoverTableManager()
// {
//     // 🗑️ Hulló anyagok táblázat kezelése
//     connect(leftoverTableManager.get(), &LeftoverTableManager::deleteRequested,
//             this, [this](const QUuid& id) {
//                 presenter->removeLeftoverEntry(id);
//             });

//     // 📝 Hulló anyagok szerkesztése
//     connect(leftoverTableManager.get(), &LeftoverTableManager::editRequested,
//             this, [this](const QUuid& id) {
//                 auto opt = LeftoverStockRegistry::instance().findById(id);
//                 if (!opt) return;

//                 LeftoverStockEntry original = *opt;

//                 AddWasteDialog dialog(this);
//                 dialog.setModel(original);

//                 if (dialog.exec() != QDialog::Accepted)
//                     return;

//                 LeftoverStockEntry updated = dialog.getModel();
//                 presenter->updateLeftoverEntry(updated);
//             });

// }
