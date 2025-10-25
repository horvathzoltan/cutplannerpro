#pragma once

#include <model/registries/cuttingplanrequestregistry.h>
#include <model/cutting/plan/request.h>

/**
 * @brief Service osztály, amely a vágási igények (Cutting::Plan::Request) listáját építi a registryből.
 *
 * Ez a réteg választja le az optimizert az éles CuttingPlanRequestRegistry-ről:
 * - az optimizer csak a kész request listát kapja,
 * - a registryhez való hozzáférés itt történik.
 *
 * Nem végez validációt, nem mutat hibát, csak a nyers adatot adja vissza.
 * A validáció és a hibakezelés a presenter feladata.
 */


class RequestSnapshotBuilder {
public:
    static QVector<Cutting::Plan::Request> build() {
        return CuttingPlanRequestRegistry::instance().readAll();
    }
};
