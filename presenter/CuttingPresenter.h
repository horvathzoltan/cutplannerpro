#pragma once

#include <QObject>
#include "../model/CuttingOptimizerModel.h"
#include "model/archivedwasteentry.h"

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
    void add_CuttingPlanRequest(const CuttingPlanRequest& req);
    void update_CuttingPlanRequest(const CuttingPlanRequest& updated);
    void remove_CuttingPlanRequest(const QUuid &id);
    //
    void removeAll_CuttingPlanRequests();

    void createNew_CuttingPlanRequests();

    void setCuttingRequests(const QVector<CuttingPlanRequest> &list);

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
    QVector<CutPlan>& getPlansRef();
    QVector<CutResult> getLeftoverResults();
    void finalizePlans();
    void scrapShortLeftovers();
    void exportArchivedWasteToCSV(const QVector<ArchivedWasteEntry> &entries);

    void syncModelWithRegistries();
    bool loadCuttingPlanFromFile(const QString &path);
private:
    MainWindow* view;
    CuttingOptimizerModel model;

    bool isModelSynced = false;
};

