// FILE: model/cutting/optimizer/piecebuildertoldas.h
#pragma once
#include "common/logger.h"
#include "model/cutting/plan/request.h"
#include "product/material_role_utils.h"
#include "materials/registry/material_registry.h"

#include <model/cutting/piece/piecewithmaterial.h>
#include <model/inventorysnapshot.h>

// NEW: ToldasEngine include
#include "model/cutting/optimizer/toldas_engine.h"

namespace Cutting {
namespace Optimizer {

class PieceBuilderToldas
{
public:
    static QVector<Cutting::Piece::PieceWithMaterial>
    buildPiecesForRequest(const Cutting::Plan::Request& req,
                          const InventorySnapshot& inv)
    {
        QVector<Cutting::Piece::PieceWithMaterial> result;

        // 0) Először megpróbáljuk a ToldasEngine-t
        {
            bool handled = false;
            ToldasEngine::computeToldasPieces(req, inv, result, handled);
            if (handled) {
                // A ToldasEngine megoldotta (toldással vagy egy darabbal)
                return result;
            }
        }

        // 1) role meghatározása
        const MaterialMaster* mm =
            MaterialRegistry::instance().findById(req.materialId);

        if (!mm)
            return result;

        MaterialRole role =
            MaterialRoleUtils::makeRole(req, mm);

        QString roleName = role.barcodePrefix;

        // 2) Nem NP-BAR esetén marad a régi, egyszerű viselkedés
        if (roleName != "NP-BAR") {
            Cutting::Piece::PieceInfo info;
            info.length_mm = req.requiredLength;
            info.requestId = req.requestId;
            info.externalReference = req.externalReference;

            result.append(Cutting::Piece::PieceWithMaterial(info, req.materialId));
            return result;
        }

        // 3) NP-BAR esetén, ha a ToldasEngine nem talált megoldást,
        //    visszaesünk a régi logikára (hulló + hulló, stock + hulló, stock + stock),
        //    vagy hibát dobunk, ha vághatatlan.
        int need = req.requiredLength;
        int stock = mm->stockLength_mm; // pl. 3000 mm

        // 3) NP-BAR esetén, ha a ToldasEngine nem talált megoldást,
        //    NEM esünk vissza a régi toldás logikára.
        //    A régi hulló+hulló, stock+hulló, stock+stock kombinációk
        //    nem kompatibilisek a szerelési workflow-val.
        //
        //    Ezért NP-BAR esetben explicit hibát dobunk,
        //    ha a ToldasEngine sem tudott megoldást adni.

        auto *mat = MaterialRegistry::instance().findById(req.materialId);
        QString matName = mat?mat->toDisplay():"?";

        zError(QString("ToldasEngine: NP-BAR igény nem toldható: need=%1, materialId=%2")
                   .arg(req.requiredLength)
                   .arg(matName));

        return result;   // üres lista → a felső réteg kezeli a hibát


        // 3/f nem toldható / nem vágható súly
        zError(QString("Súly nem toldható: need=%1, stock=%2")
                   .arg(need).arg(stock));

        return result;
    }
};

} // namespace Optimizer
} // namespace Cutting
