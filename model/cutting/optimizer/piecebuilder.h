#pragma once

#include <QHash>
#include <QVector>
#include "cuttypes.h"

namespace Cutting {
namespace Optimizer {

class PieceBuilder
{
public:
    static inline
        QHash<QUuid, QVector<Cutting::Piece::PieceWithMaterial>>
        buildPiecesByMaterial(const QVector<Cutting::Plan::Request>& requests)
    {
        QHash<QUuid, QVector<Cutting::Piece::PieceWithMaterial>> out;

        for (const Cutting::Plan::Request& req : requests) {

            int leftRemaining  = req.leftCount;
            int rightRemaining = req.rightCount;

            for (int i = 0; i < req.quantity; ++i) {

                Cutting::Piece::PieceInfo info;
                info.length_mm = req.requiredLength;
                info.requestId = req.requestId;
                info.isCompleted = false;

                // darab-sorszámozás
                if (req.quantity > 1) {
                    info.externalReference = QString("%1 %2/%3")
                    .arg(req.externalReference)
                        .arg(i + 1)
                        .arg(req.quantity);
                } else {
                    info.externalReference = req.externalReference;
                }

                // side kiosztása
                HandlerSide side = HandlerSide::None;
                if (leftRemaining > 0) {
                    side = HandlerSide::Left;
                    leftRemaining--;
                } else if (rightRemaining > 0) {
                    side = HandlerSide::Right;
                    rightRemaining--;
                }

                // PieceWithMaterial
                Cutting::Piece::PieceWithMaterial pwm(info, req.materialId);
                pwm.side = side;
                pwm.subtype = req.subtype;

                out[req.materialId].append(pwm);
            }
        }

        return out;
    }
};

} // namespace Optimizer
} // namespace Cutting
