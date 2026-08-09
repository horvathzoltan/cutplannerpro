#include "storageauditpresenter.h"
#include "../view/MainWindow.h"

#include "service/storageaudit/auditsyncguard.h"
#include "service/storageaudit/auditutils.h"

#include <service/storageaudit/leftoverauditservice.h>
#include <service/storageaudit/storageauditservice.h>


StorageAuditPresenter::StorageAuditPresenter(MainWindow* view, QObject *parent)
    : QObject(parent), _view(view) {}

void StorageAuditPresenter::runStorageAudit() {
    QVector<StorageAuditRow> stockAuditRows =
        StorageAuditService::generateAuditRows_All();
    QVector<StorageAuditRow> leftoverAuditRows =
        LeftoverAuditService::generateAuditRows_All();

    QVector<StorageAuditRow> lastAuditRows;

    lastAuditRows = stockAuditRows + leftoverAuditRows;

    // 🔁 Ha van optimalizációs terv, injektáljuk vissza
    auto cp = _view->cuttingPresenter();
    auto m = cp->optimizerModel();

    if (!m->getResult_PlansRef().isEmpty()) {
        const QVector<Cutting::Plan::CutPlan>& plans = m->getResult_PlansRef();
        AuditUtils::injectPlansIntoAuditRows(plans, &lastAuditRows);

        QMap<QUuid, int> pickingMap = AuditUtils::generatePickingMapFromPlans(plans);
        AuditUtils::assignContextsToRows(&lastAuditRows, pickingMap);
    } else {
        AuditUtils::assignContextsToRows(&lastAuditRows, {});
    }

    if (_view) {
        _view->update_StorageAuditTable(lastAuditRows);
    }

    _auditStateManager.setActiveAuditRows(lastAuditRows);

    _lastAuditRows = lastAuditRows;
}

void StorageAuditPresenter::updateRow(const QUuid& rowId,
                                 std::function<void(StorageAuditRow&)> updater)
{
    for (StorageAuditRow& row : _lastAuditRows) {
        if (row.rowId == rowId) {
            updater(row);
            if (_view) {
                _view->updateRow_StorageAuditTable(row);
            }
            break;
        }
    }
}

void StorageAuditPresenter::update_StorageAuditActualQuantity(const QUuid& rowId, int actualQuantity)
{
    auto sp = _view->storageAuditPresenter();
    auto m = sp->auditStateManager();

    AuditSyncGuard guard(m);

    updateRow(rowId, [&](StorageAuditRow& row) {
        row.actualQuantity = actualQuantity;
        row.isRowModified = (actualQuantity != row.originalQuantity);

        // 🔄 Stock frissítés
        if (auto opt = StockRegistry::instance().findById(row.stockEntryId); opt.has_value()) {
            StockEntry updated = opt.value();
            updated.quantity = actualQuantity;
            StockRegistry::instance().updateEntry(updated);

            if (_view) {
                _view->updateRow_StockTable(updated);
            }
        }
    });
}

void StorageAuditPresenter::update_StorageAuditCheckbox(const QUuid& rowId, bool checked)
{
    updateRow(rowId, [&](StorageAuditRow& row) {
        row.isRowAuditChecked = checked;
    });
}


void StorageAuditPresenter::update_LeftoverAuditActualQuantity(const QUuid& rowId, int quantity)
{
    updateRow(rowId, [&](StorageAuditRow& row) {
        if (row.sourceType != AuditSourceType::Leftover) return;

        row.actualQuantity = quantity;
        row.isRowModified = (row.actualQuantity != row.originalQuantity);
    });
}