#pragma once

#include "../../../model/cutting/instruction/cutinstruction.h"

#include <materials/model/material_master.h>

#include <materials/registry/material_registry.h>

#include <model/registries/cuttingplanrequestregistry.h>

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




inline QString formatMachineCutsEvent(const MachineCuts& mc)
{
    QStringList lines;

    lines << QString("🪚 Gép: %1").arg(mc.machineHeader.machineName);

    for (const auto& ci : mc.cutInstructions) {

        // 1) Ikon kiválasztása
        QString icon = ci.isManualCut ? "📏" : "✂️";

        // 2) Material név + kód
        const MaterialMaster* mat =
            MaterialRegistry::instance().findById(ci.materialId);

        QString materialLabel = mat
                                    ? QString("%1:%2").arg(mat->name).arg(ci.barcode)
                                    : QString("Material:%1").arg(ci.materialId.toString(QUuid::WithoutBraces));

        // 3) Request → tételszám + ownerName
        auto req = CuttingPlanRequestRegistry::instance().findById(ci.requestId);

        QString pieceLabel = req
                                 ? QString("%1. %2").arg(req->externalReference).arg(req->ownerName)
                                 : QString("req:%1").arg(ci.requestId.toString(QUuid::WithoutBraces));

        // 4) Sor összeállítása
        lines << QString("%2. %3 | %4 | %1 %5 | %6")
                     .arg(icon)
                     .arg(ci.globalStepId)
                     .arg(ci.rodId)
                     .arg(materialLabel)
                     .arg(QString::number(ci.cutSize_mm, 'f', 1))
                     .arg(pieceLabel);
    }

    return lines.join("\n");
}


}
