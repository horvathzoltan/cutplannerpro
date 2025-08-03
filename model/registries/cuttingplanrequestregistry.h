#pragma once

#include <QUuid>
#include <QVector>
#include <QHash>

#include "../cuttingplanrequest.h"

/**
 * @brief Központi registry CuttingRequest-ek tárolására és lekérdezésére.
 *
 * Egyedi anyagID-k alapján tárolja az igényeket, támogatja több igény kezelését egy anyaghoz.
 */
class CuttingPlanRequestRegistry {
private:
    CuttingPlanRequestRegistry() = default;
    CuttingPlanRequestRegistry(const CuttingPlanRequestRegistry&) = delete;

    QVector<CuttingPlanRequest> _data;
    //bool isPersist= true;

    void persist() const;
    CuttingPlanRequestRegistry& operator=(const CuttingPlanRequestRegistry&) = delete;
public:
    // 🔁 Singleton elérés
    static CuttingPlanRequestRegistry& instance();

    void registerRequest(const CuttingPlanRequest& request);        
    bool updateRequest(const CuttingPlanRequest &updated);
    void removeRequest(const QUuid &requestId);

    QVector<CuttingPlanRequest> readAll() const;
    std::optional<CuttingPlanRequest> findById(const QUuid& requestId) const; // ⬅️ új
    //QVector<CuttingPlanRequest> findByMaterialId(const QUuid& materialId) const;
    void clearAll(); // 🔄 ÚJ: teljes törlés

    bool isEmpty() const { return _data.isEmpty(); }
    void setData(const QVector<CuttingPlanRequest>& v) { _data = v;}
};
