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
public:
    static CuttingPlanRequestRegistry& instance();

    void registerRequest(const CuttingPlanRequest& request);
    QVector<CuttingPlanRequest> findByMaterialId(const QUuid& materialId) const;
    QVector<CuttingPlanRequest> readAll() const;

        void clear(); // 🔄 ÚJ: teljes törlés
    void removeRequest(const QUuid &requestId);

    bool updateRequest(const CuttingPlanRequest &updated);
    std::optional<CuttingPlanRequest> findById(const QUuid& requestId) const; // ⬅️ új
    bool isEmpty() const { return _data.isEmpty(); }
private:
    void persist() const;
    CuttingPlanRequestRegistry() = default;
    CuttingPlanRequestRegistry(const CuttingPlanRequestRegistry&) = delete;
    CuttingPlanRequestRegistry& operator=(const CuttingPlanRequestRegistry&) = delete;

    QVector<CuttingPlanRequest> _data;
};
