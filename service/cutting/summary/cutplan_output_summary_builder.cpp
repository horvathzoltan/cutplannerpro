#include "cutplan_output_summary_builder.h"
#include <model/registries/cuttingplanrequestregistry.h>
#include <materials/registry/material_registry.h>
#include <materials/utils/material_group_utils.h>

CutPlanOutputSummary CutPlanOutputSummaryBuilder::build(
    const QVector<Cutting::Plan::CutPlan>& plans,
    const QVector<Cutting::Result::ResultModel>& leftovers) const
{
    CutPlanOutputSummary s;

    //
    // Anyagonkénti összesítéshez:
    // materialId → MaterialSummary index
    //
    QHash<QUuid, int> materialIndex;

    //
    // 1) Tervek bejárása – minden CutPlan egy rúd
    //
    for (const auto& p : plans) {

        // Globális rúd darabszám
        s.rodCount++;

        // Globális rúd hossz (mm → m)
        s.rodTotal_m += p.totalLength / 1000.0;

        // Globális szegmensszám
        s.segmentCount += p.segments.size();

        //
        // Anyag szerinti összesítés előkészítése
        //
        int idx;
        if (!materialIndex.contains(p.materialId)) {

            CutPlanOutputSummary::MaterialSummary ms;
            ms.materialId        = p.materialId;
            ms.materialName      = p.materialName();
            ms.materialGroupName = p.materialGroupName();

            s.materials.append(ms);
            idx = s.materials.size() - 1;

            materialIndex.insert(p.materialId, idx);

        } else {
            idx = materialIndex.value(p.materialId);
        }

        auto& ms = s.materials[idx];

        //
        // Anyagonkénti rúdösszegzés
        //
        ms.rodCount++;
        ms.rodTotal_m += p.totalLength / 1000.0;

        //
        // Szegmensek feldolgozása
        //
        for (const auto& seg : p.segments) {

            if(seg.isPiece()){
                // Levágott darab
                s.pieceCount++;
                s.pieceTotal_m += seg.length_mm() / 1000.0;

                ms.pieceCount++;
                ms.pieceTotal_m += seg.length_mm() / 1000.0;
            } else if (seg.isKerf()){
                // Kerf
                s.kerfCount++;
                s.kerfTotal_mm += seg.length_mm();
            } else if(seg.isWaste()){
                // Hulladék
                s.wasteCount++;
                s.wasteTotal_mm += seg.length_mm();
            } else if(seg.isTechnical()){
                // Technikai szakasz – nem számít semmibe
            }
        }

        //
        // Tételszámok gyűjtése (Piece szegmensek alapján)
        //
        // for (const auto& seg : p.segments) {

        //     if (seg.isPiece())
        //         continue;

        //     auto req = CuttingPlanRequestRegistry::instance().findById(seg._requestId);
        //     if (!req)
        //         continue;

        //     if (!req->externalReference.isEmpty())
        //         ms.itemRefs.insert(req->externalReference);
        // }
        //
        // 4) Tételszámok gyűjtése – DARAB SZINTEN
        //
        for (const auto& pw : p.piecesWithMaterial) {
            const QString& ref = pw.info.externalReference;
            if (!ref.isEmpty())
                ms.itemRefs.insert(ref);
        }

    }

    //
    // 2) Leftover statisztika
    //
    for (const auto& r : leftovers) {

        if (r.waste >= 300)
            s.reusableCount++;

        if (r.isFinalWaste)
            s.archivedCount++;
    }

    //
    // 3) Hatékonyság számítása
    //
    double used_mm  = (s.pieceTotal_m * 1000.0) + s.kerfTotal_mm;
    double total_mm = (s.rodTotal_m * 1000.0);

    s.efficiency = total_mm > 0
                       ? (used_mm / total_mm) * 100.0
                       : 0.0;

    return s;
}
