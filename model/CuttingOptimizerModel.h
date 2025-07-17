#pragma once

#include <QObject>
#include <QVector>
#include <QMap>
#include "cutresult.h"
#include "cuttingrequest.h"
#include "materialregistry.h"
#include "stockentry.h"

// struct CutRequest {
//     int length;
//     int quantity;
// };

struct CutPlan {
    int rodNumber;              // ➕ Sorszám vagy index
    QVector<int> cuts;          // ✂️ A darabolt hosszok
    int kerfTotal;              // 🔧 Vágásonkénti veszteségek összege
    int waste;                  // ♻️ Használatlan anyag
    QUuid materialId;           // 🔗 Anyagtörzsbeli azonosító (helyettesíti a category-t)

    // Kényelmi metódus (opcionális)
    QString name() const {
        auto opt = MaterialRegistry::instance().findById(materialId);
        return opt ? opt->name : "(ismeretlen)";
    }

    ProfileCategory category() const {
        auto opt = MaterialRegistry::instance().findById(materialId);
        return opt ? opt->category : ProfileCategory::Unknown;
    }
};

struct PieceWithMaterial {
    int length;
    QUuid materialId;
};

// struct PieceWithCategory {
//     int length;
//     ProfileCategory category;
// };

// 💡 C++20 előtt: globális operator==
inline bool operator==(const PieceWithMaterial& a, const PieceWithMaterial& b) {
    return a.length == b.length && a.materialId == b.materialId;
}

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

    void setReusableInventory(const QVector<StockEntry> &reusable);
private:
    QVector<CuttingRequest> requests;
    //QVector<int> allPieces;
    QVector<StockEntry> profileInventory;
    QVector<StockEntry> reusableInventory;
    QVector<CutPlan> plans;
    QVector<CutResult> leftoverResults;
    //int stockLength = 6000; // mm
    int kerf = 3; // mm

    int nextOptimizationId = 1;

    QVector<int> findBestFit(const QVector<int> &available, int lengthLimit) const;

    struct ReusableCandidate {
        int indexInInventory;
        StockEntry stock;
        QVector<int> combo;
        int totalWaste;
    };

    std::optional<ReusableCandidate> findBestReusableFit(
        const QVector<StockEntry>& reusableInventory,
        const QVector<PieceWithMaterial>& pieces,
        QUuid materialId
        ) const;
};
