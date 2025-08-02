#pragma once


#include "view/MainWindow.h"

#include <model/registries/cuttingplanrequestregistry.h>

#include <view/dialog/addinputdialog.h>

#include <presenter/CuttingPresenter.h>
//#include "view/managers/inputtablemanager.h"
class InputTableConnector{
public:
    inline static void Connect(
        MainWindow *w,
         std::unique_ptr<InputTableManager>& inputTableManager,
        CuttingPresenter* presenter)
    {
        w->connect(
            inputTableManager.get(),
            &InputTableManager::deleteRequested,
            w,
            [presenter](const QUuid& id) {
                    presenter->removeCutRequest(id);
                });

        //
        w->connect(
            inputTableManager.get(),
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
                    presenter->updateCutRequest(updated);
                });
    }
};

// #include <QDialog>
// #include <QObject>
// #include <QUuid>
// #include <QWidget>
// #include <optional>

// namespace tablehelper {

// template<typename RegistryType,
//          typename DialogType,
//          typename EntryType,
//          typename PresenterType>
//     requires requires(QUuid id) {
//         { RegistryType::instance().findById(id) } -> std::convertible_to<std::optional<EntryType>>;
//     } && requires(DialogType dialog, EntryType entry) {
//         { dialog.setModel(entry) } -> std::same_as<void>;
//         { dialog.getModel() } -> std::convertible_to<EntryType>;
//     }
// void connectTableHandlers(QObject* parent,
//                           QObject* tableManager,
//                           PresenterType* presenter,
//                           void (PresenterType::*updateFn)(const EntryType&),
//                           void (PresenterType::*deleteFn)(const QUuid&))
// {
//     // Modern Qt-style connect: deleteRequested
//     QObject::connect(tableManager,
//                      static_cast<void(QObject::*)(const QUuid&)>(&QObject::deleteRequested),  // Ha ez egy QObject-ból származik
//                      parent,
//                      [=](const QUuid& id) {
//                          if (presenter && deleteFn)
//                              (presenter->*deleteFn)(id);
//                      });

//     // Modern Qt-style connect: editRequested
//     QObject::connect(tableManager,
//                      static_cast<void(QObject::*)(const QUuid&)>(&QObject::editRequested),
//                      parent,
//                      [=](const QUuid& id) {
//                          auto opt = RegistryType::instance().findById(id);
//                          if (!opt) return;

//                          EntryType original = *opt;
//                          DialogType dialog(static_cast<QWidget*>(parent));
//                          dialog.setModel(original);

//                          if (dialog.exec() != QDialog::Accepted)
//                              return;

//                          EntryType updated = dialog.getModel();
//                          if (presenter && updateFn)
//                              (presenter->*updateFn)(updated);
//                      });
// }

// } // namespace tablehelper
