#pragma once

#include <QObject>
#include <QVector>
#include <QMap>

#include <model/cutting/piece/piecewithmaterial.h>
#include "../plan/cutplan.h"
#include "../result/resultmodel.h"
#include "../plan/request.h"
#include "../../leftoverstockentry.h"
#include "../../stockentry.h"

namespace Cutting {
namespace Optimizer {

class OptimizerModel : public QObject {
    Q_OBJECT

public:
    explicit OptimizerModel(QObject *parent = nullptr);

    void setKerf(int kerf);

    QVector<Cutting::Plan::CutPlan> &getResult_PlansRef();
    QVector<Cutting::Result::ResultModel> getResults_Leftovers() const;

    void optimize();

    void setCuttingRequests(const QVector<Cutting::Plan::Request>& list);
    void setStockInventory(const QVector<StockEntry> &list);
    void setReusableInventory(const QVector<LeftoverStockEntry> &reusable);

private:
    QVector<Cutting::Plan::Request> requests;
    QVector<StockEntry> profileInventory;
    QVector<LeftoverStockEntry> reusableInventory;

    QVector<Cutting::Plan::CutPlan> _result_plans;
    QVector<Cutting::Result::ResultModel> _result_leftovers;

    int kerf = 3; // mm

    int nextOptimizationId = 1;

    QVector<Cutting::Piece::PieceWithMaterial> findBestFit(const QVector<Cutting::Piece::PieceWithMaterial>& available, int lengthLimit) const;

    struct ReusableCandidate {
        int indexInInventory;
        LeftoverStockEntry stock;
        QVector<Cutting::Piece::PieceWithMaterial> combo;
        int totalWaste;
    };

    std::optional<ReusableCandidate> findBestReusableFit(
        const QVector<LeftoverStockEntry>& reusableInventory,
        const QVector<Cutting::Piece::PieceWithMaterial>& pieces,
        QUuid materialId
        ) const;
};

} //end namespace Optimizer
} //end namespace Cutting
