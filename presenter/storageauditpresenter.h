#pragma once

#include "model/cutting/plan/cutplan.h"
#include "service/storageaudit/auditstatemanager.h"
#include <QObject>

class MainWindow; // Előre deklaráljuk, hogy ne kelljen most includolni


class StorageAuditPresenter : public QObject {
    Q_OBJECT
public:
    explicit StorageAuditPresenter(MainWindow* view, QObject *parent = nullptr);

    void runStorageAudit();

    AuditStateManager* auditStateManager() { return &_auditStateManager;}
    QVector<StorageAuditRow>& lastAuditRows() {return _lastAuditRows;}

    void updateRow(const QUuid &rowId, std::function<void (StorageAuditRow &)> updater);
    void update_StorageAuditActualQuantity(const QUuid &rowId, int actualQuantity);
    void update_StorageAuditCheckbox(const QUuid &rowId, bool checked);
    void update_LeftoverAuditActualQuantity(const QUuid &rowId, int quantity);

private:
    MainWindow* _view;
    AuditStateManager _auditStateManager;
    QVector<StorageAuditRow> _lastAuditRows;
};
