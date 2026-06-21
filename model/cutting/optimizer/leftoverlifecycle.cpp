#include "leftoverlifecycle.h"
#include "common/logger.h"
#include "common/identifierutils.h"
#include "settings/settingsmanager.h"
#include "model/cutting/optimizer/lineagehelper.h"

#include <materials/registry/material_registry.h>

namespace Cutting {
namespace Optimizer {
namespace LeftoverLifecycle {

LeftoverStockEntry createPhysicalLeftover(
    const SelectedRod& rod,
    int remainingLength,
    int currentOpId,
    const QVector<Cutting::Plan::CutPlan>& resultPlans,
    bool& created)
{
    created = false;

    zInfo(QString("🔍 FIZIKAI HULLÓ KERESÉSE — remaining=%1 mm").arg(remainingLength));
    if (remainingLength <= 0)
        return {};

    const MaterialMaster* mat = MaterialRegistry::instance().findById(rod.materialId);
    MaterialTrimmingParams tp = mat ? mat->trimmingParams(rod.isReusable)
                                    : MaterialTrimmingParams::getDefault();

    int corrected = remainingLength - tp.frontTrim_mm - tp.backTrim_mm - tp.minLeftOver_mm;
    if (corrected <= 0) {
        zInfo(QString("✖ Fizikai hulló nem hozható létre — corrected=%1 mm").arg(corrected));
        return {};
    }

    LeftoverStockEntry entry;
    entry.materialId = rod.materialId;
    entry.availableLength_mm = corrected;
    entry.used = false;
    entry.barcode = IdentifierUtils::makeLeftoverId(SettingsManager::instance().nextMaterialCounter());
    entry._parent = rod._parent;
    entry.source = Cutting::Result::LeftoverSource::Optimization;
    entry.optimizationId = std::make_optional(currentOpId);

    zInfo(QString("🎯 Fizikai hulló létrehozva — barcode=%1, length=%2 mm, rodId=%3, parent=%4")
              .arg(entry.barcode)
              .arg(entry.availableLength_mm)
              .arg(rod.rodId)
              .arg(entry._parent ? entry._parent->toString() : "—"));

    LineageHelper::validateLineage(entry, resultPlans);
    zInfo(LineageHelper::lineageTree(entry, resultPlans));

    created = true;
    return entry;
}

} // namespace LeftoverLifecycle
} // namespace Optimizer
} // namespace Cutting
