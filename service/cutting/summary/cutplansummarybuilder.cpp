#include "cutplan_input_summary_builder.h"
#include "cutplan_output_summary_builder.h"
#include "cutplansummarybuilder.h"
//#include "materials/utils/material_group_utils.h"

#include <model/registries/cuttingplanrequestregistry.h>

#include <materials/model/material_master.h>

#include <materials/registry/material_registry.h>


CutPlanSummary CutPlanSummaryBuilder::build(
    const QVector<Cutting::Plan::CutPlan>& plans,
    const QVector<Cutting::Result::ResultModel>& leftovers,
    const QString& planIdStr)
{
    CutPlanSummary s;
    s.input  = CutPlanInputSummaryBuilder().build();
    s.output = CutPlanOutputSummaryBuilder().build(plans, leftovers);
    s.planIdStr = planIdStr;
    return s;
}



// CutPlanSummary CutPlanSummaryBuilder::build(
//     const QVector<Cutting::Plan::CutPlan>& plans,
//     const QVector<Cutting::Result::ResultModel>& leftovers)
// {
//     CutPlanSummary s;


//     // ÚJ: Gyártási input összefoglaló
//     auto requests = CuttingPlanRequestRegistry::instance().readAll();
//     QHash<QString, int> ownerIndex;

//     for (const auto& r : requests) {
//         int idx;
//         if (!ownerIndex.contains(r.ownerName)) {
//             CutPlanSummary::RequestSummary rs;
//             rs.ownerName = r.ownerName;
//             s.inputSummaries.append(rs);
//             idx = s.inputSummaries.size() - 1;
//             ownerIndex.insert(r.ownerName, idx);
//         } else {
//             idx = ownerIndex.value(r.ownerName);
//         }

//         auto& rs = s.inputSummaries[idx];

//         // anyag szerinti blokk
//         int matIdx = -1;
//         for (int i = 0; i < rs.materials.size(); ++i) {
//             if (rs.materials[i].materialId == r.materialId) {
//                 matIdx = i;
//                 break;
//             }
//         }
//         if (matIdx == -1) {
//             CutPlanSummary::RequestSummary::MaterialBlock mb;
//             mb.materialId = r.materialId;

//             // itt jön a request → material törzs
//             const MaterialMaster* mat =
//                 MaterialRegistry::instance().findById(r.materialId);

//             if (mat) {
//                 mb.materialName      = mat->name;
//                 mb.materialBarcode   = mat->barcode;
//                 mb.materialGroupName =  GroupUtils::groupName(mat->id);
//             } else {
//                 mb.materialName      = r.materialId.toString();
//                 mb.materialBarcode   = {};
//                 mb.materialGroupName = {};
//             }

//             rs.materials.append(mb);
//             matIdx = rs.materials.size() - 1;
//         }

//         auto& mb = rs.materials[matIdx];


//         CutPlanSummary::RequestSummary::Item it;
//         it.externalRef = r.externalReference + ".";
//         it.length_mm = r.requiredLength;
//         it.quantity = r.quantity;

//         mb.items.append(it);

//         rs.totalItems++;
//         rs.totalQuantity += r.quantity;

//         s.totalInputItems++;
//         s.totalInputQuantity += r.quantity;
//     }


//     //
//     // 🔢 Globális rúd darabszám
//     //
//     s.rodCount = plans.size();

//     //
//     // 📦 Anyagonkénti összesítéshez:
//     // materialId → MaterialSummary index
//     //
//     QHash<QUuid, int> materialIndex;



//     //
//     // 📊 Tervek bejárása – minden CutPlan egy rúd
//     //
//     for (const auto& p : plans) {

//         //
//         // 📏 Globális rúd hossz (mm → m)
//         //
//         s.rodTotal_m += p.totalLength / 1000.0;

//         //
//         // 🧱 Globális szegmensszám
//         //
//         s.segmentCount += p.segments.size();



//         //
//         // 📦 Anyag szerinti összesítés előkészítése
//         //
//         int idx;
//         if (!materialIndex.contains(p.materialId)) {

//             // Új anyaghoz új MaterialSummary blokk
//             CutPlanSummary::MaterialSummary ms;
//             ms.materialId        = p.materialId;
//             ms.materialName      = p.materialName();
//             ms.materialGroupName = p.materialGroupName();

//             s.materials.push_back(ms);
//             idx = s.materials.size() - 1;

//             materialIndex.insert(p.materialId, idx);

//         } else {
//             idx = materialIndex.value(p.materialId);
//         }

//         auto& ms = s.materials[idx];



//         //
//         // 📏 Anyagonkénti rúdösszegzés
//         //
//         ms.rodCount++;
//         ms.rodTotal_m += p.totalLength / 1000.0;



//         //
//         // ✂️ Szegmensek feldolgozása – a SegmentModel a fizikai igazság
//         //
//         for (const auto& seg : p.segments) {

//             switch (seg.type()) {

//             case Cutting::Segment::SegmentModel::Type::Piece:
//                 //
//                 // ✂️ Levágott darab
//                 //
//                 s.pieceCount++;
//                 s.pieceTotal_m += seg.length_mm() / 1000.0;

//                 ms.pieceCount++;
//                 ms.pieceTotal_m += seg.length_mm() / 1000.0;
//                 break;


//             case Cutting::Segment::SegmentModel::Type::Kerf:
//                 //
//                 // 🔧 Kerf (vágási veszteség)
//                 //
//                 s.kerfCount++;
//                 s.kerfTotal_mm += seg.length_mm();
//                 break;


//             case Cutting::Segment::SegmentModel::Type::Waste:
//                 //
//                 // 🪓 Hulladék szegmens
//                 //
//                 s.wasteCount++;
//                 s.wasteTotal_mm += seg.length_mm();
//                 break;


//             case Cutting::Segment::SegmentModel::Type::Technical:
//                 //
//                 // 🛠️ Technikai szakasz:
//                 // - nem darab
//                 // - nem kerf
//                 // - nem hulladék
//                 // - nem leftover
//                 // - nem része a hatékonyságnak
//                 // - nem része semmilyen statisztikának
//                 //
//                 break;
//             }
//         }



//         //
//         // 🧾 Anyagonkénti tételszámok gyűjtése a SZEGMENSEK alapján
//         // (ez a fizikai igazság, minden Piece szegmens hordozza a requestId-t)
//         //
//         for (const auto& seg : p.segments) {

//             if (seg.type() != Cutting::Segment::SegmentModel::Type::Piece)
//                 continue;

//             // 1) Request visszakeresése
//             auto req = CuttingPlanRequestRegistry::instance().findById(seg._requestId);
//             if (!req)
//                 continue;

//             // 2) Tételszám hozzáadása
//             if (!req->externalReference.isEmpty())
//                 ms.itemRefs.insert(req->externalReference);
//         }
//     }



//     //
//     // ♻️ Leftover statisztika – valós üzleti logika
//     //
//     for (const auto& r : leftovers) {

//         // Újrahasználható leftover (>= 300 mm)
//         if (r.waste >= 300)
//             s.reusableCount++;

//         // Archivált végmaradék
//         if (r.isFinalWaste)
//             s.archivedCount++;
//     }



//     //
//     // 🚦 Hatékonyság számítása
//     //
//     double used_mm  = (s.pieceTotal_m * 1000.0) + s.kerfTotal_mm;
//     double total_mm = (s.rodTotal_m * 1000.0);

//     s.efficiency = total_mm > 0
//                        ? (used_mm / total_mm) * 100.0
//                        : 0.0;



//     return s;
// }

