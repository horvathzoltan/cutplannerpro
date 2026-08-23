#pragma once

#include "../../model/registries/cuttingplanrequestregistry.h"
#include "../../model/cutting/plan/request.h"
#include "materials/utils/material_group_utils.h"
#include "product/utils/material_role_utils.h"
#include <materials/registry/material_registry.h>
#include <calculation/lengthcalculator.h>
#include <product/registry/product_subtype_registry.h>
#include <product/registry/product_type_registry.h>

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

        QVector<Cutting::Plan::Request> list =
            CuttingPlanRequestRegistry::instance().readAll();

        for (Cutting::Plan::Request& r : list) {

            const MaterialMaster* m =
                MaterialRegistry::instance().findById(r.materialId);

            if (!m)
                continue;

            MaterialRole role =
                MaterialRoleUtils::makeRole(r, m);

            auto type = ProductTypeRegistry::instance().findById(r.productTypeId);
            auto subtype = ProductSubtypeRegistry::instance().findById(r.productSubtypeId);

            if(type && subtype){
                auto comp = LengthCalculator::compensate(
                    type->code,
                    subtype->code,
                    r.attributes,
                    role.barcodePrefix);

                if (comp.has_value()) {
                    r.requiredLength += *comp;
                }
            }
        }

        return list;
    }

    static QMap<QUuid, QVector<int>> getLengthsPerMaterial(const QVector<Cutting::Plan::Request>& requests){

        QMap<QUuid, QVector<int>> reqLengths;
        for (const auto& r : requests) {
            // quantity-szer kell hozzáadni
            for (int i = 0; i < r.quantity; ++i) {
                reqLengths[r.materialId].append(r.requiredLength);
            }
        }
        return reqLengths;
    }

    static QMap<QUuid, QVector<int>>
    expandLengthsWithGroupMembers(const QMap<QUuid, QVector<int>>& lengthsPerMaterial)
    {
        QMap<QUuid, QVector<int>> expanded = lengthsPerMaterial;

        for (auto it = lengthsPerMaterial.begin(); it != lengthsPerMaterial.end(); ++it) {

            QUuid materialId = it.key();
            const QVector<int>& lengths = it.value();

            // 1️⃣ Group tagok lekérése
            QSet<QUuid> siblings = GroupUtils::groupMembers(materialId);

            // 2️⃣ Minden group‑taghoz bemásoljuk a hosszlistát
            for (const QUuid& sibId : siblings) {

                // Ha már van ilyen anyag a mapben → nem írjuk felül
                if (expanded.contains(sibId))
                    continue;

                expanded[sibId] = lengths;
            }
        }

        return expanded;
    }


};
