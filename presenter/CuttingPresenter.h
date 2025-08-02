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
    void addCutRequest(const CuttingPlanRequest& req);
    void updateCutRequest(const CuttingPlanRequest& updated);
    void removeCutRequest(const QUuid &id);

    void createNewCuttingPlan();

    void setCuttingRequests(const QVector<CuttingPlanRequest> &list);

    // Készlet
    void setStockInventory(const QVector<StockEntry> &list);

    // Úrafelhasználható - hulló anyagok készlete
    void setReusableInventory(const QVector<LeftoverStockEntry> &list);

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

    void clearCuttingPlan();

    void removeStockEntry(const QUuid &stockId);
    void updateStockEntry(const StockEntry &updated);

    void removeLeftoverEntry(const QUuid &entryId);
    void updateLeftoverEntry(const LeftoverStockEntry &updated);
    void syncModelWithRegistries();
private:
    MainWindow* view;
    CuttingOptimizerModel model;

    bool isModelSynced = false;
};

