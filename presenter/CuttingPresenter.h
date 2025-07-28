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
    void addCutRequest(const CuttingRequest& req);
    void updateCutRequest(const CuttingRequest& updated);


    void setCuttingRequests(const QVector<CuttingRequest> &list);
    void clearRequests();

    // Készlet
    void setStockInventory(const QVector<StockEntry> &list);

    // Úrafelhasználható - hulló anyagok készlete
    void setReusableInventory(const QVector<ReusableStockEntry> &list);

    // Paraméterek
    void setKerf(int kerf);

    // Optimalizálás
    void runOptimization();

    // Eredmények lekérése
    QVector<CutPlan> getPlans();
    QVector<CutResult> getLeftoverResults();
    void finalizePlans();
    void scrapShortLeftovers();
    void exportArchivedWasteToCSV(const QVector<ArchivedWasteEntry> &entries);
    void removeCutRequest(const QUuid &id);
    private:
    MainWindow* view;
    CuttingOptimizerModel model;
};

