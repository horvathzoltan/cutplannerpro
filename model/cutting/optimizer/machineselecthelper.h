#pragma once

#include <optional>
#include <QString>
#include "../../../common/logger.h"
#include "materials/registry/material_registry.h"
#include "../../machine/machineutils.h"
#include "cuttypes.h"

namespace Cutting {
namespace Optimizer {

class MachineSelectHelper
{
public:
    static inline std::optional<CuttingMachine> pickAndLog(const QUuid& materialId)
    {
        const MaterialMaster* mat =
            MaterialRegistry::instance().findById(materialId);

        zInfo(QString("🔍 GÉP KERESÉSE — anyag=%1")
                  .arg(mat ? mat->toDisplay() : materialId.toString()));

        auto machineOpt = MachineUtils::pickMachineForMaterial(materialId);

        if (!machineOpt) {
            zInfo("✖ Nincs kompatibilis gép — pending darab eldobása");
            return std::nullopt;
        }

        const CuttingMachine& machine = *machineOpt;

        zInfo(QString("✔ Kiválasztott gép: %1 (kerf=%2 mm, maxLen=%3)")
                  .arg(machine.name)
                  .arg(machine.kerf_mm)
                  .arg(machine.stellerMaxLength_mm.has_value()
                           ? QString::number(*machine.stellerMaxLength_mm)
                           : "n/a"));

        zInfo(QString("   • Gép kerf érték: %1 mm").arg(machine.kerf_mm));

        return machineOpt;
    }
};

} // namespace Optimizer
} // namespace Cutting
