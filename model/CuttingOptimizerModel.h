#pragma once

#include <QObject>
#include <QVector>
#include <QMap>

#include <model/cutting/piecewithmaterial.h>
#include "cutplan.h"
#include "cutresult.h"
#include "cuttingplanrequest.h"
#include "leftoverstockentry.h"
#include "stockentry.h"

class CuttingOptimizerModel : public QObject {
    Q_OBJECT

public:
    explicit CuttingOptimizerModel(QObject *parent = nullptr);

    void setKerf(int kerf);

    QVector<CutPlan> &getResult_PlansRef();
    QVector<CutResult> getResults_Leftovers() const;

    void optimize();

    void setCuttingRequests(const QVector<CuttingPlanRequest>& list);
    void setStockInventory(const QVector<StockEntry> &list);
    void setReusableInventory(const QVector<LeftoverStockEntry> &reusable);

private:
    QVector<CuttingPlanRequest> requests;
    QVector<StockEntry> profileInventory;
    QVector<LeftoverStockEntry> reusableInventory;

    QVector<CutPlan> _result_plans;
    QVector<CutResult> _result_leftovers;

    int kerf = 3; // mm

    int nextOptimizationId = 1;

    QVector<PieceWithMaterial> findBestFit(const QVector<PieceWithMaterial>& available, int lengthLimit) const;

    struct ReusableCandidate {
        int indexInInventory;
        LeftoverStockEntry stock;
        QVector<PieceWithMaterial> combo;
        int totalWaste;
    };

    std::optional<ReusableCandidate> findBestReusableFit(
        const QVector<LeftoverStockEntry>& reusableInventory,
        const QVector<PieceWithMaterial>& pieces,
        QUuid materialId
        ) const;
};


