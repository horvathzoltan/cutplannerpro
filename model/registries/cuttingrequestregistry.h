#pragma once

#include <QUuid>
#include <QVector>
#include <QHash>

#include "../cuttingrequest.h"

/**
 * @brief Központi registry CuttingRequest-ek tárolására és lekérdezésére.
 *
 * Egyedi anyagID-k alapján tárolja az igényeket, támogatja több igény kezelését egy anyaghoz.
 */
class CuttingRequestRegistry {
public:
    static CuttingRequestRegistry& instance();

    void registerRequest(const CuttingRequest& request);
    QVector<CuttingRequest> findByMaterialId(const QUuid& materialId) const;
    QVector<CuttingRequest> readAll() const;

        void clear(); // 🔄 ÚJ: teljes törlés
    void removeRequest(const QUuid &requestId);

    bool updateRequest(const CuttingRequest &updated);
    std::optional<CuttingRequest> findById(const QUuid& requestId) const; // ⬅️ új

private:
    void persist() const;
    CuttingRequestRegistry() = default;
    CuttingRequestRegistry(const CuttingRequestRegistry&) = delete;
    CuttingRequestRegistry& operator=(const CuttingRequestRegistry&) = delete;

    QVector<CuttingRequest> _requests;
};
