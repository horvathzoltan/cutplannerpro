#pragma once

#include "model/cutting/instruction/cutinstruction.h"

namespace CuttingInstructionUtils {

enum class SortStrategy {
    BySizeDesc,
    ByMaterialThenSize,
    //ByStatus,
    None
};


inline void postProcessMachineCuts(MachineCuts& mc, SortStrategy strategy = SortStrategy::BySizeDesc) {

    // 1️⃣ Rendezés a stratégiának megfelelően
    switch (strategy) {
    case SortStrategy::BySizeDesc:
        std::sort(mc.cutInstructions.begin(), mc.cutInstructions.end(),
                  [](const CutInstruction& a, const CutInstruction& b){
                      return a.cutSize_mm > b.cutSize_mm;
                  });
        break;

    case SortStrategy::ByMaterialThenSize:
        std::stable_sort(mc.cutInstructions.begin(), mc.cutInstructions.end(),
                         [](const CutInstruction& a, const CutInstruction& b){
                             if (a.materialId == b.materialId)
                                 return a.cutSize_mm > b.cutSize_mm;
                             return a.materialId.toString() < b.materialId.toString();
                         });
        break;

    // case SortStrategy::ByStatus:
    //     std::stable_sort(mc.cutInstructions.begin(), mc.cutInstructions.end(),
    //                      [](const CutInstruction& a, const CutInstruction& b){
    //                          return static_cast<int>(a.status) < static_cast<int>(b.status);
    //                      });
    //     break;

    case SortStrategy::None:
        // nincs rendezés
        break;
    }

    // 2️⃣ Kompenzációs logika
    double comp = mc.machineHeader.stellerCompensation_mm.value_or(0.0);
    double maxLen = mc.machineHeader.stellerMaxLength_mm.value_or(-1);

    for (auto& ci : mc.cutInstructions) {
        ci.isManualCut = false;
        ci.effectiveCutSize_mm = ci.cutSize_mm;

        if (maxLen <= 0) {
            ci.isManualCut = true;
        } else {
            double withComp = ci.cutSize_mm + comp;
            if (withComp > maxLen) {
                ci.isManualCut = true;
            } else {
                ci.effectiveCutSize_mm = withComp;
            }
        }
    }
}

}
