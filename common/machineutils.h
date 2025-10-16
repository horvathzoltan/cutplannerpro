#pragma once

#include <optional>
#include <QVector>
#include <QUuid>
#include <algorithm>

#include "model/cutting/cuttingmachine.h"
#include "model/registries/cuttingmachineregistry.h"
#include "model/registries/materialregistry.h"
#include "common/eventlogger.h"

namespace MachineUtils {

/**
 * @brief Anyag alapján kiválaszt egy kompatibilis gépet a CuttingMachineRegistry-ből.
 *
 * Preferencia sorrend:
 *  1. Legkisebb kerf_mm
 *  2. Ha azonos, akkor legnagyobb stellerMaxLength_mm
 *  3. Ha azonos, akkor név szerinti sorrend
 *
 * @param materialId Az anyag UUID-je
 * @return std::optional<CuttingMachine> A kiválasztott gép, vagy std::nullopt ha nincs kompatibilis
 */
inline std::optional<CuttingMachine> pickMachineForMaterial(const QUuid& materialId) {
    // 1. Anyag lekérése
    const auto matOpt = MaterialRegistry::instance().findById(materialId);
    if (!matOpt) {
        EventLogger::instance().zEvent(
            QString("❌ Anyag nem található a registry-ben: %1").arg(materialId.toString()));
        return std::nullopt;
    }
    MaterialType type = matOpt->type;

    // 2. Gépek lekérése
    const auto& machines = CuttingMachineRegistry::instance().readAll();
    QVector<const CuttingMachine*> candidates;
    for (const auto& m : machines) {
        if (m.compatibleMaterials.contains(type)) {
            candidates.append(&m);
        }
    }

    if (candidates.isEmpty()) {
        EventLogger::instance().zEvent(
            QString("⚠️ Nincs kompatibilis gép az anyaghoz: %1").arg(matOpt->name));
        return std::nullopt;
    }

    // 3. Preferencia sorrend
    std::sort(candidates.begin(), candidates.end(),
              [](const CuttingMachine* a, const CuttingMachine* b) {
                  if (a->kerf_mm != b->kerf_mm)
                      return a->kerf_mm < b->kerf_mm;
                  if (a->stellerMaxLength_mm && b->stellerMaxLength_mm)
                      return *a->stellerMaxLength_mm > *b->stellerMaxLength_mm;
                  return a->name < b->name;
              });

    const CuttingMachine* chosen = candidates.front();
    EventLogger::instance().zEvent(
        QString("✅ Gép kiválasztva: %1 (kerf=%2 mm)")
            .arg(chosen->name)
            .arg(chosen->kerf_mm));

    return *chosen;
}

} // namespace MachineUtils
