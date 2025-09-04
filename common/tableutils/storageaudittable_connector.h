#pragma once

#include "view/MainWindow.h"
//#include "view/dialog/addwastedialog.h"
//#include "view/dialog/stock/editstoragedialog.h"
#include <model/registries/stockregistry.h>
#include <model/registries/cuttingplanrequestregistry.h>
#include <model/registries/leftoverstockregistry.h>
#include <view/dialog/addinputdialog.h>
#include <presenter/CuttingPresenter.h>

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
               [presenter](const QUuid& entryId, int actualQuantity) {
                   presenter->update_StorageAuditActualQuantity(entryId, actualQuantity);
               });
}
}
