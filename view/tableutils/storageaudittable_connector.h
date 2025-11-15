#pragma once

#include "../MainWindow.h"
//#include "view/dialog/addwastedialog.h"
//#include "view/dialog/stock/editstoragedialog.h"
#include "../../model/registries/stockregistry.h"
#include "../../model/registries/cuttingplanrequestregistry.h"
#include "../../model/registries/leftoverstockregistry.h"
#include "../dialog/input/addinputdialog.h"
#include "../../presenter/CuttingPresenter.h"

namespace StorageAuditTableConnector {

inline static void Connect (
    MainWindow* w,
    StorageAuditTableManager* manager,
    CuttingPresenter* presenter)
{
    // 📊 Audit érték változás
    w->connect(manager,
               &StorageAuditTableManager::auditValueChanged,
               w,
               [presenter](const QUuid& rowId, int actualQuantity) {
                   presenter->update_StorageAuditActualQuantity(rowId, actualQuantity);
               });

    w->connect(manager,
               &StorageAuditTableManager::leftoverQuantityChanged,
               w,
               [presenter](const QUuid& rowId, int quantity) {
                   presenter->update_LeftoverAuditActualQuantity(rowId, quantity);
               });
    w->connect(manager,
               &StorageAuditTableManager::auditCheckboxToggled,
               w,
               [presenter](const QUuid& rowId, bool checked) {
                   presenter->update_StorageAuditCheckbox(rowId, checked);
               });

}
}
