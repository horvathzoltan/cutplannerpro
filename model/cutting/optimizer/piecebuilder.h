#pragma once

#include "piecebuilder_toldas.h"

#include <QHash>
#include <QVector>
#include <materials/registry/material_registry.h>
#include <model/cutting/piece/piecewithmaterial.h>
#include <model/cutting/plan/request.h>
#include <model/inventorysnapshot.h>

namespace Cutting {
namespace Optimizer {

class PieceBuilder
{
public:
    static inline
        QHash<QUuid, QVector<Cutting::Piece::PieceWithMaterial>>
        buildPiecesByMaterial(const QVector<Cutting::Plan::Request>& requests,
                          const InventorySnapshot& inventorySnapshot)
    {
        QHash<QUuid, QVector<Cutting::Piece::PieceWithMaterial>> out;

        for (const Cutting::Plan::Request& req : requests)
        {

            auto *mat = MaterialRegistry::instance().findById(req.materialId);
            QString matName = mat?mat->barcode:"?";

            zInfo(L("[buildPiecesByMaterial] material: %1, %2 mm : %3 db")
                  .arg(matName)
                  .arg(req.requiredLength)
                  .arg(req.quantity));

            int leftRemaining  = req.leftCount;
            int rightRemaining = req.rightCount;

             // for (int i = 0; i < req.quantity; ++i) {
             //     Cutting::Piece::PieceInfo info;
             //     info.length_mm = req.requiredLength;
             //     info.requestId = req.requestId;
             //     info.isCompleted = false;

             //     // darab-sorszámozás
             //     if (req.quantity > 1) {
             //         info.externalReference = QString("%1 %2/%3")
             //         .arg(req.externalReference)
             //             .arg(i + 1)
             //             .arg(req.quantity);
             //     } else {
             //         info.externalReference = req.externalReference;
             //     }

             //     // side kiosztása
             //     HandlerSide side = HandlerSide::None;
             //     if (leftRemaining > 0) {
             //         side = HandlerSide::Left;
             //         leftRemaining--;
             //     } else if (rightRemaining > 0) {
             //         side = HandlerSide::Right;
             //         rightRemaining--;
             //     }

             //     zInfo(L("[buildPiecesByMaterial] extref: %1, side: %2")
             //           .arg(info.externalReference)
             //           .arg(HandlerSideUtils::toDisplayText(side)));

             //     // PieceWithMaterial
             //     Cutting::Piece::PieceWithMaterial pwm(info, req.materialId);
             //     pwm.side = side;
             //     //pwm.subtype = req.subtype;

             //     pwm.productTypeId = req.productTypeId;
             //     pwm.productSubtypeId = req.productSubtypeId;
             //     pwm.attributes = req.attributes;

             //     out[req.materialId].append(pwm);
             // }


           for (int i = 0; i < req.quantity; ++i) {

               auto pieces = PieceBuilderToldas::buildPiecesForRequest(req, inventorySnapshot);

               QString externalRef;
               if (req.quantity > 1) {
                   externalRef = QString("%1 %2/%3")
                   .arg(req.externalReference)
                       .arg(i + 1)
                       .arg(req.quantity);
               } else {
                   externalRef = req.externalReference;
               }

               HandlerSide side = HandlerSide::None;
               if (leftRemaining > 0) {
                   side = HandlerSide::Left;
                   leftRemaining--;
               } else if (rightRemaining > 0) {
                   side = HandlerSide::Right;
                   rightRemaining--;
               }

               for (auto& p : pieces) {

                   p.info.externalReference = externalRef;
                   p.side = side;

                   p.productTypeId = req.productTypeId;
                   p.productSubtypeId = req.productSubtypeId;
                   p.attributes = req.attributes;

                   out[req.materialId].append(p);
               }
           }

        }

        return out;
    }



}; //end of class

} // namespace Optimizer
} // namespace Cutting
