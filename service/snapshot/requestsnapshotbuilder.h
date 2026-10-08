#pragma once

#include "../../model/cutting/plan/request.h"
#include "materials/utils/material_group_utils.h"
#include "product/utils/material_role_utils.h"
#include <materials/registry/material_registry.h>
#include <materials/registry/material_rolegroup_registry.h>
#include <calculation/lengthcalculator.h>
#include <product/registry/material_role_registry.h>
#include <product/registry/product_subtype_registry.h>
#include <product/registry/product_type_registry.h>

/**
 * @brief Request alapú optimalizálási inputokat építő helper.
 *
 * Feladata:
 * - requestek hosszlistává alakítása
 * - materialonkénti aggregálás
 * - helyettesítő anyagcsoportok expandálása
 *
 * Nem végez validációt és nem ér el készletadatokat.
 */


class RequestSnapshotBuilder {
public:
    enum class MaterialDemandOrigin
    {
        Request,
        Expanded
    };


    struct MaterialLengthDemand
    {
        QUuid materialId;

        MaterialDemandOrigin origin =
            MaterialDemandOrigin::Request;

        QUuid originMaterialId;

        QVector<int> lengths;
    };

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

    static QMap<QUuid, MaterialLengthDemand>
    expandLengthsWithGroupMembers(const QMap<QUuid, QVector<int>>& lengthsPerMaterial)
    {
        QMap<QUuid, MaterialLengthDemand> expanded;

        for (auto it = lengthsPerMaterial.begin(); it != lengthsPerMaterial.end(); ++it) {

            QUuid materialId = it.key();
            const QVector<int>& lengths = it.value();
            // önmaga beillesztése
            expanded[materialId] = MaterialLengthDemand{
                .materialId = materialId,
                .origin = MaterialDemandOrigin::Request,
                .originMaterialId = materialId,
                .lengths = lengths
            };

            // 1️⃣ Group tagok lekérése
            QSet<QUuid> siblings = GroupUtils::groupMembers(materialId);

            // 2️⃣ Minden group‑taghoz bemásoljuk a hosszlistát
            for (const QUuid& sibId : siblings) {

                // Ha már van ilyen anyag a mapben → nem írjuk felül
                if (expanded.contains(sibId))
                    continue;

                expanded[sibId] = MaterialLengthDemand{
                    .materialId = sibId,
                    .origin = MaterialDemandOrigin::Expanded,
                    .originMaterialId = materialId,
                    .lengths = lengths
                };
            }
        }

        return expanded;
    }

};
