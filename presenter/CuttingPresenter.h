#pragma once

#include <QObject>
#include "../model/cutting/optimizer/optimizermodel.h"
#include "common/auditstatemanager.h"
#include "model/archivedwasteentry.h"
#include "model/relocation/relocationinstruction.h"
#include "model/storageaudit/storageauditrow.h"

/*
Értelmezi és kezeli a felhasználói interakciókat

Meghívja a modell metódusait (pl. optimize())

Átadja a View-nak a megjelenítendő adatokat (pl. vágási terv, maradékok)
*/
class MainWindow; // Előre deklaráljuk, hogy ne kelljen most includolni

class CuttingPresenter : public QObject {
    Q_OBJECT

public:
    explicit CuttingPresenter(MainWindow* view, QObject *parent = nullptr);

    // Vágási igények
    void add_CuttingPlanRequest(const Cutting::Plan::Request& req);
    void update_CuttingPlanRequest(const Cutting::Plan::Request& updated);
    void remove_CuttingPlanRequest(const QUuid &id);
    //
    void removeAll_CuttingPlanRequests();

    void createNew_CuttingPlanRequests();

    void setCuttingRequests(const QVector<Cutting::Plan::Request> &list);

    // Készlet
    void setStockInventory(const QVector<StockEntry> &list);

    void add_StockEntry(const StockEntry &entry);
    void remove_StockEntry(const QUuid &stockId);
    void update_StockEntry(const StockEntry &updated);

    // Úrafelhasználható - hulló anyagok készlete
    void setReusableInventory(const QVector<LeftoverStockEntry> &list);
    void add_LeftoverStockEntry(const LeftoverStockEntry& entry);
    void remove_LeftoverStockEntry(const QUuid &entryId);
    void update_LeftoverStockEntry(const LeftoverStockEntry &updated);

    // Paraméterek
    void setKerf(int kerf);

    // Optimalizálás
    void runOptimization();

    // Eredmények lekérése
    QVector<Cutting::Plan::CutPlan>& getPlansRef();
    QVector<Cutting::Result::ResultModel> getLeftoverResults();
    void finalizePlans();
    void scrapShortLeftovers();
    void exportArchivedWasteToCSV(const QVector<ArchivedWasteEntry> &entries);

    void syncModelWithRegistries();
    bool loadCuttingPlanFromFile(const QString &path);
    void runStorageAudit(const QMap<QString, int>& pickingMap);

    QVector<RelocationInstruction> generateRelocationPlan(
        const QVector<StorageAuditRow>& auditRows,
        const QString& cuttingZoneName);

    const QVector<StorageAuditRow>& getLastAuditRows() const { return lastAuditRows;}

    void update_StorageAuditActualQuantity(const QUuid &rowId, int actualQuantity);

    AuditStateManager* auditStateManager() { return &_auditStateManager;}

    void update_LeftoverAuditPresence(const QUuid &rowId, AuditPresence presence);
private:
    MainWindow* view;
    Cutting::Optimizer::OptimizerModel model;
    QVector<StorageAuditRow> lastAuditRows;

    bool isModelSynced = false;
    QMap<QString, int> generatePickingMapFromPlans(const QVector<Cutting::Plan::CutPlan> &plans);
    void logPlans();
    AuditStateManager _auditStateManager;

};

