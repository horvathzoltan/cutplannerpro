#ifndef CUTTINGPRESENTER_H
#define CUTTINGPRESENTER_H

#include <QObject>
#include "../model/CuttingOptimizerModel.h"

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

    void addRequest(const CuttingRequest& req);
    void setKerf(int kerf);
    void clearRequests();
    void runOptimization();

    void setCuttingRequests(const QVector<CuttingRequest> &list);
    void setStockInventory(const QVector<StockEntry> &list);

    QVector<CutPlan> getPlans();
    QVector<CutResult> getLeftoverResults();
private:
    MainWindow* view;
    CuttingOptimizerModel model; 
};

#endif // CUTTINGPRESENTER_H
