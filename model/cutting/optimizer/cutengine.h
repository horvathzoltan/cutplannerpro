#pragma once

#include "../piece/piecewithmaterial.h"
#include "../cuttingmachine.h"
#include "selectedrod.h"
#include "cuttypes.h"

class CutEngine {
public:
    static CutResult cutSingle(
        const Cutting::Piece::PieceWithMaterial& piece,
        int remainingLength,
        const SelectedRod& rod,
        const CuttingMachine& machine,
        int currentOpId,
        int rodId,
        double kerf_mm,
        int dpLimit,
        int& planCounter);

    static CutResult cutCombo(
        const QVector<Cutting::Piece::PieceWithMaterial>& combo,
        int remainingLength,
        const SelectedRod& rod,
        const CuttingMachine& machine,
        int currentOpId,
        int rodId,
        double kerf_mm,
        int dpLimit,
        int& planCounter);

};
