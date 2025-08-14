#pragma once

#include <QUuid>
#include <QVector>
#include <QHash>

#include "../cutting/plan/request.h"

/**
 * @brief Központi registry CuttingRequest-ek tárolására és lekérdezésére.
 *
 * Egyedi anyagID-k alapján tárolja az igényeket, támogatja több igény kezelését egy anyaghoz.
 */
class CuttingPlanRequestRegistry {
private:
    CuttingPlanRequestRegistry() = default;
    CuttingPlanRequestRegistry(const CuttingPlanRequestRegistry&) = delete;

    QVector<Cutting::Plan::Request> _data;
    //bool isPersist= true;

    void persist() const;
    CuttingPlanRequestRegistry& operator=(const CuttingPlanRequestRegistry&) = delete;
public:
    // 🔁 Singleton elérés
    static CuttingPlanRequestRegistry& instance();

    void registerRequest(const Cutting::Plan::Request& request);
    bool updateRequest(const Cutting::Plan::Request &updated);
    void removeRequest(const QUuid &requestId);

    QVector<Cutting::Plan::Request> readAll() const;
    std::optional<Cutting::Plan::Request> findById(const QUuid& requestId) const; // ⬅️ új
    //QVector<CuttingPlanRequest> findByMaterialId(const QUuid& materialId) const;
    void clearAll(); // 🔄 ÚJ: teljes törlés

    bool isEmpty() const { return _data.isEmpty(); }
    void setData(const QVector<Cutting::Plan::Request>& v) { _data = v;}
};
