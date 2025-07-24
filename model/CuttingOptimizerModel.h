#pragma once

#include <QObject>
#include <QVector>
#include <QMap>

#include <model/cutting/piecewithmaterial.h>
//#include "common/grouputils.h"
#include "cutplan.h"
#include "cutresult.h"
#include "cuttingrequest.h"
//#include "registries/materialregistry.h"
//#include "model/cutting/piecewithmaterial.h"
#include "reusablestockentry.h"
#include "stockentry.h"

// // struct CutRequest {
// //     int length;
// //     int quantity;
// // };

// struct CutPlan {
//     int rodNumber;              // ➕ Sorszám vagy index
//     QVector<int> cuts;          // ✂️ A darabolt hosszok
//     int kerfTotal;              // 🔧 Vágásonkénti veszteségek összege
//     int waste;                  // ♻️ Használatlan anyag
//     QUuid materialId;           // 🔗 Anyagtörzsbeli azonosító (helyettesíti a category-t)

//     QString rodId;

//     // Kényelmi metódus (opcionális)
//     QString name() const {
//         auto opt = MaterialRegistry::instance().findById(materialId);
//         return opt ? opt->name : "(ismeretlen)";
//     }

//     QString groupName() const {
//         return GroupUtils::groupName(materialId);
//     }
// };

// struct PieceWithMaterial {
//      int length;
//      QUuid materialId;
// };

// 💡 C++20 előtt: globális operator==


class CuttingOptimizerModel : public QObject {
    Q_OBJECT

public:
    explicit CuttingOptimizerModel(QObject *parent = nullptr);

    void clearRequests();
    void addRequest(const CuttingRequest& req);
    void setKerf(int kerf);
    void optimize();

    QVector<CutPlan> getPlans() const;
    QVector<CutResult> getLeftoverResults() const;

    void setLeftovers(const QVector<CutResult> &list);
    void clearLeftovers();

    // 🔹 Teljes darablista legyártása kategória szerint

    void setRequests(const QVector<CuttingRequest>& list);
    void setStockInventory(const QVector<StockEntry> &list);
    void setReusableInventory(const QVector<ReusableStockEntry> &reusable);
    QVector<CutPlan> &getPlansRef();
private:
    QVector<CuttingRequest> requests;
    //QVector<int> allPieces;
    QVector<StockEntry> profileInventory;
    QVector<ReusableStockEntry> reusableInventory;
    QVector<CutPlan> plans;
    QVector<CutResult> leftoverResults;
    //int stockLength = 6000; // mm
    int kerf = 3; // mm

    int nextOptimizationId = 1;

    //QVector<int> findBestFit(const QVector<int> &available, int lengthLimit) const;
    QVector<PieceWithMaterial> findBestFit(const QVector<PieceWithMaterial>& available, int lengthLimit) const;


    struct ReusableCandidate {
        int indexInInventory;
        ReusableStockEntry stock;
        QVector<PieceWithMaterial> combo;
        int totalWaste;
    };

    std::optional<ReusableCandidate> findBestReusableFit(
        const QVector<ReusableStockEntry>& reusableInventory,
        const QVector<PieceWithMaterial>& pieces,
        QUuid materialId
        ) const;
};
