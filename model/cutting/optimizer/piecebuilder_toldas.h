// FILE: model/cutting/optimizer/piecebuildertoldas.h
#pragma once
#include "common/logger.h"
#include "model/cutting/plan/request.h"
#include "product/material_role_utils.h"
#include "materials/registry/material_registry.h"

#include <model/cutting/piece/piecewithmaterial.h>
#include <model/inventorysnapshot.h>
#include <model/registries/leftoverstockregistry.h>

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

            zInfo(QString("PieceBuilderToldas: ToldasEngine returned handled=%1, pieces=%2")
                      .arg(handled)
                      .arg(result.size()));

            for (const auto& p : result) {
                auto l = LeftoverStockRegistry::instance().findById(*p.info.leftoverEntryId);
                QString ltxt = l.has_value()?(l->barcode):"?";

                zInfo(QString("  → PB piece: len=%1, role=%2, leftover=%3")
                          .arg(p.info.length_mm)
                          .arg(p.info.toldasRole)
                          .arg(ltxt));
            }


            if (handled) {
                // A ToldasEngine megoldotta (toldással vagy egy darabbal)
                return result;
            }

            // if (handled) {

            //     QVector<Cutting::Piece::PieceWithMaterial> finalPieces;

            //     for (const auto& p : result) {

            //         // 1) TOLDAS_MAIN darab → NEM megy a FitEngine-be
            //         //    hanem közvetlenül a CutPlan-ba kerül (single-cut)
            //         // if (p.info.toldasRole == "TOLDAS_MAIN") {

            //         //     Cutting::Piece::PieceInfo mainInfo = p.info;
            //         //     mainInfo.keepWhole = true;          // fontos: egyben kiadandó
            //         //     mainInfo.leftoverEntryId.reset();   // nem leftover
            //         //     mainInfo.toldasRole = "TOLDAS_MAIN";

            //         //     finalPieces.append(Cutting::Piece::PieceWithMaterial(mainInfo, p.materialId));
            //         //     continue;
            //         // }

            //         // 2) TOLDAS_TOLDAT → FitEngine-be megy (vágandó)
            //         finalPieces.append(p);
            //     }

            //     return finalPieces;
            // }

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

        zWarning(QString("ToldasEngine: NP-BAR igény nem toldható: need=%1, materialId=%2")
                   .arg(req.requiredLength)
                   .arg(matName));

        // 3) NP-BAR esetén, ha a ToldasEngine nem talált megoldást,
        //    NE térjünk vissza üres listával → különben eltűnik a súly.
        //    Adjunk vissza egy sima darabot fallbackként.

        // auto *mat = MaterialRegistry::instance().findById(req.materialId);
        // QString matName = mat ? mat->toDisplay() : "?";

        zWarning(QString("ToldasEngine: NP-BAR igény nem toldható: need=%1, materialId=%2 → FALLBACK: sima darab")
                     .arg(req.requiredLength)
                     .arg(matName));

        // Fallback: sima darab
        Cutting::Piece::PieceInfo info;
        info.length_mm = req.requiredLength;
        info.requestId = req.requestId;
        info.externalReference = req.externalReference;
        info.toldasRole = "";   // fontos: nem toldott darab

        result.append(Cutting::Piece::PieceWithMaterial(info, req.materialId));
        return result;

        // 3/f nem toldható / nem vágható súly
        // zError(QString("Súly nem toldható: need=%1, stock=%2")
        //            .arg(need).arg(stock));

        // return result;
    }
};

} // namespace Optimizer
} // namespace Cutting
