#include "cutplan_input_summary_builder.h"
#include <model/registries/cuttingplanrequestregistry.h>
#include <materials/registry/material_registry.h>
#include <materials/utils/material_group_utils.h>

CutPlanInputSummary CutPlanInputSummaryBuilder::build() const
{
    CutPlanInputSummary s;

    auto requests = CuttingPlanRequestRegistry::instance().readAll();

    // ownerName → OwnerBlock index
    QHash<QString, int> ownerIndex;

    for (const auto& r : requests) {

        //
        // 1) OwnerBlock kiválasztása vagy létrehozása
        //
        int oIdx;
        if (!ownerIndex.contains(r.ownerName)) {
            CutPlanInputSummary::OwnerBlock ob;
            ob.ownerName = r.ownerName;
            s.owners.append(ob);
            oIdx = s.owners.size() - 1;
            ownerIndex.insert(r.ownerName, oIdx);
        } else {
            oIdx = ownerIndex.value(r.ownerName);
        }

        auto& owner = s.owners[oIdx];

        //
        // 2) MaterialBlock kiválasztása vagy létrehozása
        //
        int mIdx = -1;
        for (int i = 0; i < owner.materials.size(); ++i) {
            if (owner.materials[i].materialId == r.materialId) {
                mIdx = i;
                break;
            }
        }

        if (mIdx == -1) {
            CutPlanInputSummary::MaterialBlock mb;
            mb.materialId = r.materialId;

            // anyag törzsből
            const MaterialMaster* mat =
                MaterialRegistry::instance().findById(r.materialId);

            if (mat) {
                mb.materialName      = mat->name;
                mb.materialBarcode   = mat->barcode;
            } else {
                mb.materialName      = r.materialId.toString();
                mb.materialBarcode   = "";
            }

            // csoportnév a MaterialGroupRegistry-ből
            mb.materialGroupName = GroupUtils::groupName(r.materialId);

            owner.materials.append(mb);
            mIdx = owner.materials.size() - 1;
        }

        auto& mb = owner.materials[mIdx];

        //
        // 3) Item hozzáadása
        //
        CutPlanInputSummary::Item it;
        it.externalRef = r.externalReference;   // pont nélkül
        it.length_mm   = r.requiredLength;
        it.quantity    = r.quantity;

        mb.items.append(it);

        //
        // 4) Összesítések
        //
        mb.totalItems++;
        mb.totalQuantity += r.quantity;
    }

    return s;
}
