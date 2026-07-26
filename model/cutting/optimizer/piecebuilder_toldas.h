#pragma once
#include "common/logger.h"
#include "model/cutting/plan/request.h"
#include "product/material_role_utils.h"
#include "materials/registry/material_registry.h"

#include <model/cutting/piece/piecewithmaterial.h>

#include <model/inventorysnapshot.h>

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

        // 1) role meghatározása
        const MaterialMaster* mm =
            MaterialRegistry::instance().findById(req.materialId);

        if (!mm)
            return result;

        MaterialRole role =
            MaterialRoleUtils::makeRole(req, mm);

        QString roleName = role.barcodePrefix;


        // 2) csak NP-BAR (súly) toldás
        if (roleName != "NP-BAR") {
            Cutting::Piece::PieceInfo info;
            info.length_mm = req.requiredLength;
            info.requestId = req.requestId;
            info.externalReference = req.externalReference;

            result.append(Cutting::Piece::PieceWithMaterial(info, req.materialId));
            return result;
        }

        // 3) súly toldás
        int need = req.requiredLength;
        int stock = mm->stockLength_mm; // pl. 3000 mm

        // 3/a sima darab
        if (need <= stock) {
            Cutting::Piece::PieceInfo info;
            info.length_mm = need;
            info.requestId = req.requestId;
            info.externalReference = req.externalReference;

            result.append(Cutting::Piece::PieceWithMaterial(info, req.materialId));
            return result;
        }

        // 3/b hullók lekérdezése
        QVector<LeftoverStockEntry> leftovers;
        for (const auto& l : inv.reusableInventory) {
            if (l.materialId == req.materialId)
                leftovers.append(l);
        }

        // 3/c hulló + hulló
        for (const auto& l1 : leftovers)
            for (const auto& l2 : leftovers)
                if (l1.entryId != l2.entryId &&
                    l1.availableLength_mm + l2.availableLength_mm >= need)
                {
                    Cutting::Piece::PieceInfo info1;
                    info1.length_mm = l1.availableLength_mm;
                    info1.requestId = req.requestId;
                    info1.externalReference = req.externalReference;

                    result.append(Cutting::Piece::PieceWithMaterial(info1, req.materialId));

                    Cutting::Piece::PieceInfo info2;
                    info2.length_mm = need - l1.availableLength_mm;
                    info2.requestId = req.requestId;
                    info2.externalReference = req.externalReference;

                    result.append(Cutting::Piece::PieceWithMaterial(info2, req.materialId));

                    return result;
                }

        // 3/d stock + hulló
        for (const auto& l : leftovers)
            if (stock + l.availableLength_mm >= need)
            {
                Cutting::Piece::PieceInfo info1;
                info1.length_mm = stock;
                info1.requestId = req.requestId;
                info1.externalReference = req.externalReference;

                result.append(Cutting::Piece::PieceWithMaterial(info1, req.materialId));

                Cutting::Piece::PieceInfo info2;
                info2.length_mm = need - stock;
                info2.requestId = req.requestId;
                info2.externalReference = req.externalReference;

                result.append(Cutting::Piece::PieceWithMaterial(info2, req.materialId));

                return result;
            }

        // 3/e stock + stock
        if (stock * 2 >= need) {

            Cutting::Piece::PieceInfo info1;
            info1.length_mm = stock;
            info1.requestId = req.requestId;
            info1.externalReference = req.externalReference;

            result.append(Cutting::Piece::PieceWithMaterial(info1, req.materialId));

            Cutting::Piece::PieceInfo info2;
            info2.length_mm = need - stock;
            info2.requestId = req.requestId;
            info2.externalReference = req.externalReference;

            result.append(Cutting::Piece::PieceWithMaterial(info2, req.materialId));

            return result;
        }


        // 3/f nem toldható
        zError(QString("Súly nem toldható: need=%1, stock=%2")
                   .arg(need).arg(stock));

        return result;
    }
};

} // Optimizer
} // Cutting
