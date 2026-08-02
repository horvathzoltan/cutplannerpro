#pragma once

#include <QVector>
#include "kitting/model/kittinginstruction.h"
#include "model/cutting/plan/request.h"
#include "model/cutting/piece/piecewithmaterial.h"
#include "model/cutting/plan/cutplan.h"

class KittingEngine {
public:
    static QVector<KittingInstruction> expand(
        const Cutting::Plan::Request& req,
        const Cutting::Piece::PieceWithMaterial& pwm,
        const Cutting::Plan::CutPlan& plan);
};
