#include "cutengine.h"
#include "common/eventlogger.h"
#include "model/cutting/result/resultmodel.h"
#include "service/cutting/optimizer/optimizerutils.h"

CutResult CutEngine::cutSingle(
    const Cutting::Piece::PieceWithMaterial& piece,
    int remainingLength,
    const SelectedRod& rod,
    const CuttingMachine& machine,
    int currentOpId,
    int rodId,
    double kerf_mm,
    int& planCounter)
{
    CutResult cr;
    cr.status = CutResultStatus::Unknown;
    cr.planId = QUuid();
    cr.used = 0;
    cr.waste = 0;

    int used = piece.info.length_mm + OptimizerUtils::roundKerfLoss(1, kerf_mm);

    if (used > remainingLength) {
        cr.status = CutResultStatus::Overfill;
        return cr;
    }

    int waste = remainingLength - used;

    Cutting::Plan::CutPlan p;
    p.planNumber = planCounter++;
    p.piecesWithMaterial = { piece };
    p.kerfTotal = OptimizerUtils::roundKerfLoss(1, kerf_mm);
    p.waste = waste;
    p.materialId = rod.materialId;
    p.rodId = rod.rodId;
    p.source = rod.isReusable ? Cutting::Plan::Source::Reusable : Cutting::Plan::Source::Stock;
    p.planId = QUuid::createUuid();
    p.status = Cutting::Plan::Status::NotStarted;
    p.totalLength = rod.length;
    p.machineId = machine.id;
    p.machineName = machine.name;
    p.kerfUsed_mm = kerf_mm;
    p.generateSegments(kerf_mm, rod.length);
    p.sourceBarcode = rod.barcode;
    p.optimizationId = currentOpId;
    p.parentBarcode = rod.barcode;
    p.parentPlanId = std::nullopt;
    p.leftoverBarcode = p.getWasteBarcode();

    Cutting::Result::ResultModel result;
    result.cutPlanId = p.planId;
    result.materialId = rod.materialId;
    result.length = rod.length;
    result.cuts = { piece };
    result.waste = waste;
    result.source = rod.isReusable
                        ? Cutting::Result::ResultSource::FromReusable
                        : Cutting::Result::ResultSource::FromStock;
    result.optimizationId = rod.isReusable ? std::nullopt : std::make_optional(currentOpId);
    result.reusableBarcode = p.leftoverBarcode;
    result.isFinalWaste = (waste <= 0);
    result.parentBarcode = p.parentBarcode;
    result.sourceBarcode = rod.barcode;

    cr.status = CutResultStatus::Ok;
    cr.planId = p.planId;
    cr.used = used;
    cr.waste = waste;
    cr.plan = p;
    cr.result = result;
    cr.usedPieceIds = { piece.info.pieceId };

    return cr;
}

CutResult CutEngine::cutCombo(
    const QVector<Cutting::Piece::PieceWithMaterial>& combo,
    int remainingLength,
    const SelectedRod& rod,
    const CuttingMachine& machine,
    int currentOpId,
    int rodId,
    double kerf_mm,
    int& planCounter)
{
    CutResult cr;
    cr.status = CutResultStatus::Unknown;
    cr.used = 0;
    cr.waste = 0;

    int totalCut  = OptimizerUtils::sumLengths(combo);
    int kerfTotal = OptimizerUtils::roundKerfLoss(combo.size(), kerf_mm);
    int used      = totalCut + kerfTotal;

    if (used > remainingLength) {
        cr.status = CutResultStatus::Overfill;
        return cr;
    }

    int waste = remainingLength - used;

    Cutting::Plan::CutPlan p;
    p.planNumber = planCounter++;
    p.piecesWithMaterial = combo;
    p.kerfTotal = kerfTotal;
    p.waste = waste;
    p.materialId = rod.materialId;
    p.rodId = rod.rodId;
    p.source = rod.isReusable ? Cutting::Plan::Source::Reusable : Cutting::Plan::Source::Stock;
    p.planId = QUuid::createUuid();
    p.status = Cutting::Plan::Status::NotStarted;
    p.totalLength = rod.length;
    p.machineId = machine.id;
    p.machineName = machine.name;
    p.kerfUsed_mm = kerf_mm;
    p.generateSegments(kerf_mm, rod.length);
    p.sourceBarcode = rod.barcode;
    p.optimizationId = currentOpId;
    p.parentBarcode = rod.barcode;
    p.parentPlanId = std::nullopt;
    p.leftoverBarcode = p.getWasteBarcode();

    Cutting::Result::ResultModel result;
    result.cutPlanId = p.planId;
    result.materialId = rod.materialId;
    result.length = rod.length;
    result.cuts = combo;
    result.waste = waste;
    result.source = rod.isReusable
                        ? Cutting::Result::ResultSource::FromReusable
                        : Cutting::Result::ResultSource::FromStock;
    result.optimizationId = rod.isReusable ? std::nullopt : std::make_optional(currentOpId);
    result.reusableBarcode = p.leftoverBarcode;
    result.isFinalWaste = (waste <= 0);
    result.parentBarcode = p.parentBarcode;
    result.sourceBarcode = rod.barcode;

    cr.status = CutResultStatus::Ok;
    cr.planId = p.planId;
    cr.used = used;
    cr.waste = waste;
    cr.plan = p;
    cr.result = result;

    // több darab → több pieceId
    for (auto& pc : combo)
        cr.usedPieceIds.append(pc.info.pieceId);

    return cr;
}



// OptimizerModel::CutResult OptimizerModel::cutSinglePieceBatch(
//     const Cutting::Piece::PieceWithMaterial& piece,
//     int& remainingLength,
//     const SelectedRod& rod,
//     const CuttingMachine& machine,
//     int currentOpId,
//     int rodId,
//     double kerf_mm,
//     QVector<Cutting::Piece::PieceWithMaterial>& groupVec)
// {
//     CutResult cr;
//     cr.status = CutStatus::Unknown;
//     cr.planId = QUuid();
//     cr.used = 0;
//     cr.waste = 0;

//     int used = piece.info.length_mm + OptimizerUtils::roundKerfLoss(1, kerf_mm);

//     zInfo(QString("CUT: rodId=%1, barcode=%2, totalLength=%3, remainingBefore=%4, used=%5")
//               .arg(rod.rodId)
//               .arg(rod.barcode)
//               .arg(rod.length)
//               .arg(remainingLength)
//               .arg(used));

//     // 🔒 Overfill-védelem
//     if (used > remainingLength) {
//         zError(QString("❌ Overfill detected in cutSinglePieceBatch: used=%1 > remaining=%2 (rodId=%3)")
//                    .arg(used).arg(remainingLength).arg(rod.rodId));
//         //remainingLength = 0;
//         cr.status = CutStatus::Overfill;
//         return cr;
//     }

//     int waste = OptimizerUtils::computeWasteInt(remainingLength, used);

//     // 📦 CutPlan
//     Cutting::Plan::CutPlan p;
//     p.planNumber = planCounter++;   // 🔢 Globális sorszám kiosztása
//     p.piecesWithMaterial = { piece };
//     p.kerfTotal = OptimizerUtils::roundKerfLoss(1, kerf_mm);
//     p.waste = remainingLength - used;  // tényleges maradék a vágás után;
//     p.materialId = rod.materialId;
//     p.rodId = rod.rodId;  // mindig a SelectedRod rodId-ja (öröklődik vagy új generált)
//     p.source = rod.isReusable ? Cutting::Plan::Source::Reusable : Cutting::Plan::Source::Stock;
//     p.planId = QUuid::createUuid();
//     p.status = Cutting::Plan::Status::NotStarted;
//     p.totalLength = rod.length;   // mindig a teljes rúd hossza;
//     //p.totalLength = remainingLength + used;   // effektív hossz
//     p.machineId   = machine.id;
//     p.machineName = machine.name;
//     p.kerfUsed_mm = kerf_mm;

//     // ez a cutplan piecesWithMaterial-ből csinál szegmenseket, a maradék az waste
//     //p.generateSegments(kerf_mm, remainingLength);

//     // ez a cutplan piecesWithMaterial-ből csinál szegmenseket, a maradék az waste
//     // PATCH #7 — a szegmensgenerálás mindig a teljes rúd hosszára történik
//     p.generateSegments(kerf_mm, rod.length);

//     p.sourceBarcode = rod.barcode;   // MAT-xxx vagy RST-xxx
//     p.optimizationId = currentOpId;

//     p.parentBarcode = rod.barcode;

//     // if (rod.isReusable) {
//     //     p.parentBarcode = rod.barcode;
//     // } else {
//     //     p.parentBarcode = std::nullopt;
//     // }

//     p.parentPlanId  = std::nullopt; // később, ha láncolni akarod

//     QString wasteBarcode = p.getWasteBarcode();
//     p.leftoverBarcode = wasteBarcode;

//     zEvent(OptimizerUtils::formatCutPlanEvent(p, machine));


//     // ➕ ResultModel
//     Cutting::Result::ResultModel result;
//     result.cutPlanId = p.planId;
//     result.materialId = rod.materialId;
//     result.length = rod.length;
//     result.cuts = { piece };
//     result.waste = waste;
//     result.source = rod.isReusable
//                         ? Cutting::Result::ResultSource::FromReusable
//                         : Cutting::Result::ResultSource::FromStock;
//     result.optimizationId = rod.isReusable
//                                 ? std::nullopt
//                                 : std::make_optional(currentOpId);
//     result.reusableBarcode = wasteBarcode;
//     result.isFinalWaste = (remainingLength - used <= 0);
//     result.parentBarcode = p.parentBarcode; // 🔗 auditlánc
//     result.sourceBarcode = rod.barcode;     // 🔗 eredeti rúd


//     // ♻️ Leftover visszarakása
//     if (!result.isFinalWaste && result.waste > 0 && used <= remainingLength + used) {
//         // ⚠️ FONTOS: a LeftoverStockEntry default CTOR már generál egyedi entryId-t.
//         // Itt TILOS újra beállítani, mert a konstruktorban már létrejött a GUID.
//         LeftoverStockEntry entry;

//         entry.materialId = result.materialId;
//         entry.availableLength_mm = result.waste;
//         entry.used = false;
//         entry.barcode = result.reusableBarcode;
//         entry.parentBarcode = rod.barcode;
//         // entry.parentBarcode = rod.isReusable
//         //                           ? p.parentBarcode
//         //                           : std::nullopt;
//         entry.source = Result::LeftoverSource::Optimization;
//         entry.optimizationId = std::make_optional(currentOpId);

//         //zEvent(OptimizerUtils::formatLeftoverEvent(entry, rod.rodId));

//         zInfo(QString("♻️ Assigned RST barcode=%1 for leftover entryId=%2 (rodId=%3)")
//                   .arg(entry.barcode)
//                   .arg(entry.entryId.toString())
//                   .arg(rod.rodId));


//          // 🔑 Regisztráljuk a leftover → rodId kapcsolatot
//          leftoverRodMap.insert(entry.entryId, rod.rodId);
//          zInfo(QString("MAP-INSERT: %1 → %2")
//                           .arg(entry.entryId.toString())
//                           .arg(rod.rodId));

//          // Csak utána tesszük be a listába
//          _localLeftovers.append(entry);

//          // ⛔ A forrás leftover tiltása, nem az új waste-é
//          if (rod.isReusable) {
//              if (rod.entryId.has_value()) {
//                  _usedLeftoverEntryIds.insert(rod.entryId.value());
//                  zInfo(QString("TILTÁS: forrás leftover entryId=%1 (rodId=%2)")
//                            .arg(rod.entryId->toString())
//                            .arg(rod.rodId));
//              } else {
//                  zError(QString("❌ Inconsistent state: reusable rod without entryId (rodId=%1, barcode=%2)")
//                             .arg(rod.rodId)
//                             .arg(rod.barcode));
//                  Q_ASSERT(false);// vagy return; hogy ne folytassa
//              }
//          }

//          // 🔍 ParentBarcode log
//          zInfo(QString("ParentBarcode set in ResultModel=%1, entry.parentBarcode=%2")
//                    .arg(result.parentBarcode.value_or("∅"))
//                    .arg(entry.parentBarcode.value_or("∅")));
//     }

//     _planned_leftovers.append(result);
//     _result_plans.append(p);

//     // Darab törlése
//     groupVec.erase(std::remove_if(groupVec.begin(), groupVec.end(),
//                                   [&](const auto& candidate){
//                                       return candidate.info.pieceId == piece.info.pieceId;
//                                   }), groupVec.end());

//     zInfo(QString("CUT-END: rodId=%1, remainingAfter=%2")
//               .arg(rod.rodId)
//               .arg(remainingLength - used));

//     cr.status = CutStatus::Ok;
//     cr.planId = p.planId;
//     cr.used = used;
//     cr.waste = waste;
//     return cr;

// }

// OptimizerModel::CutResult OptimizerModel::cutComboBatch(
//     const QVector<Cutting::Piece::PieceWithMaterial>& combo,
//     int& remainingLength,
//     const SelectedRod& rod,
//     const CuttingMachine& machine,
//     int currentOpId,
//     int rodId,
//     double kerf_mm,
//     QVector<Cutting::Piece::PieceWithMaterial>& groupVec)
// {

//     CutResult cr;
//     cr.status = CutStatus::Unknown;
//     cr.planId = QUuid();
//     cr.used = 0;
//     cr.waste = 0;


//     int totalCut  = OptimizerUtils::sumLengths(combo);
//     int kerfTotal = OptimizerUtils::roundKerfLoss(combo.size(), kerf_mm);
//     int used      = totalCut + kerfTotal;

//     zInfo(QString("CUT-COMBO: rodId=%1, barcode=%2, totalLength=%3, remainingBefore=%4, used=%5")
//               .arg(rod.rodId)
//               .arg(rod.barcode)
//               .arg(rod.length)
//               .arg(remainingLength)
//               .arg(used));


//     // 🔒 Overfill-védelem
//     if (used > remainingLength) {
//         zError(QString("❌ Overfill detected in cutComboBatch: used=%1 > remaining=%2 (rodId=%3)")
//                    .arg(used).arg(remainingLength).arg(rod.rodId));
//         //remainingLength = 0;
//         cr.status = CutStatus::Overfill; // a batch érvénytelen → nem vágunk
//         return cr;
//     }


//     int waste     = OptimizerUtils::computeWasteInt(remainingLength, used);

//     // 📦 CutPlan
//     Cutting::Plan::CutPlan p;
//     //p.rodNumber = rodId;
//     p.planNumber = planCounter++;   // 🔢 Globális sorszám kiosztása
//     p.piecesWithMaterial = combo;
//     p.kerfTotal = kerfTotal;
//     p.waste =  remainingLength - used;  // tényleges maradék a vágás után;
//     p.materialId = rod.materialId;
//     p.rodId = rod.rodId;  // mindig a SelectedRod rodId-ja
//     p.source = rod.isReusable ? Cutting::Plan::Source::Reusable : Cutting::Plan::Source::Stock;
//     p.planId = QUuid::createUuid();
//     p.status = Cutting::Plan::Status::NotStarted;
//     p.totalLength = rod.length;   // mindig a teljes rúd hossza;
//     //p.totalLength = remainingLength + used;   // effektív hossz
//     p.machineId   = machine.id;
//     p.machineName = machine.name;
//     p.kerfUsed_mm = kerf_mm;
//     //p.generateSegments(kerf_mm, remainingLength);

//     // PATCH #7 — a szegmensgenerálás mindig a teljes rúd hosszára történik
//     p.generateSegments(kerf_mm, rod.length);

//     p.sourceBarcode = rod.barcode;   // MAT-xxx vagy RST-xxx
//     p.optimizationId = currentOpId;

//     p.parentBarcode = rod.barcode;

//     // if (rod.isReusable) {
//     //     p.parentBarcode = rod.barcode;
//     // } else {
//     //     p.parentBarcode = std::nullopt;
//     // }
//     p.parentPlanId  = std::nullopt; // később, ha láncolni akarod

//     QString wasteBarcode = p.getWasteBarcode();

//     p.leftoverBarcode = wasteBarcode;

//     zEvent(OptimizerUtils::formatCutPlanEvent(p, machine));

//     // ➕ ResultModel
//     Cutting::Result::ResultModel result;
//     result.cutPlanId = p.planId;
//     result.materialId = rod.materialId;
//     result.length = rod.length;
//     result.cuts = combo;
//     result.waste = waste;
//     result.source = rod.isReusable
//                         ? Cutting::Result::ResultSource::FromReusable
//                         : Cutting::Result::ResultSource::FromStock;
//     result.optimizationId = rod.isReusable
//                                 ? std::nullopt
//                                 : std::make_optional(currentOpId);
//     result.reusableBarcode = wasteBarcode;
//     result.isFinalWaste = (remainingLength - used <= 0);
//     result.parentBarcode = p.parentBarcode; // 🔗 auditlánc
//     result.sourceBarcode = rod.barcode;     // 🔗 eredeti rúd

//     if (!result.isFinalWaste && result.waste > 0 && used <= remainingLength + used) {
//         // ⚠️ FONTOS: a LeftoverStockEntry default CTOR már generál egyedi entryId-t.
//         // Itt TILOS újra beállítani, mert a konstruktorban már létrejött a GUID.
//         LeftoverStockEntry entry;

//         entry.materialId = result.materialId;
//         entry.availableLength_mm = result.waste;
//         entry.used = false;
//         entry.barcode = result.reusableBarcode;
//         entry.parentBarcode = rod.barcode;/*rod.isReusable
//                                   ? p.parentBarcode
//                                   : std::nullopt;*/
//         entry.source = Result::LeftoverSource::Optimization;
//         entry.optimizationId = std::make_optional(currentOpId);


//         //zEvent(OptimizerUtils::formatLeftoverEvent(entry, rod.rodId));

//         zInfo(QString("♻️ Assigned RST barcode=%1 for leftover entryId=%2 (rodId=%3)")
//                   .arg(entry.barcode)
//                   .arg(entry.entryId.toString())
//                   .arg(rod.rodId));


//         // 🔑 Regisztráljuk a leftover → rodId kapcsolatot
//         leftoverRodMap.insert(entry.entryId, rod.rodId);
//         zInfo(QString("MAP-INSERT: %1 → %2")
//                   .arg(entry.entryId.toString())
//                   .arg(rod.rodId));

//         // Csak utána tesszük be a listába
//         _localLeftovers.append(entry);

//         // ⛔ A forrás leftover tiltása, nem az új waste-é
//         if (rod.isReusable) {
//             if (rod.entryId.has_value()) {
//                 _usedLeftoverEntryIds.insert(rod.entryId.value());
//                 zInfo(QString("TILTÁS: forrás leftover entryId=%1 (rodId=%2)")
//                           .arg(rod.entryId->toString())
//                           .arg(rod.rodId));
//             } else {
//                 zError(QString("❌ Inconsistent state: reusable rod without entryId (rodId=%1, barcode=%2)")
//                            .arg(rod.rodId)
//                            .arg(rod.barcode));
//                 Q_ASSERT(false); //vagy return; hogy ne folytassa
//             }
//         }

//         // 🔍 ParentBarcode log
//         zInfo(QString("ParentBarcode set in ResultModel=%1, entry.parentBarcode=%2")
//                   .arg(result.parentBarcode.value_or("∅"))
//                   .arg(entry.parentBarcode.value_or("∅")));
//     }

//     _result_plans.append(p);
//     _planned_leftovers.append(result);


//     // Darabok törlése
//     groupVec.erase(std::remove_if(groupVec.begin(), groupVec.end(),
//                                   [&](const auto& candidate){
//                                       return std::any_of(combo.begin(), combo.end(),
//                                                          [&](const auto& used){
//                                                              return candidate.info.pieceId == used.info.pieceId;
//                                                          });
//                                   }), groupVec.end());

//     zInfo(QString("CUT-COMBO-END: rodId=%1, remainingAfter=%2")
//               .arg(rod.rodId)
//               .arg(remainingLength - used));

//     cr.status = CutStatus::Ok;
//     cr.planId = p.planId;
//     cr.used = used;
//     cr.waste = waste;
//     return cr;

// }


// CutResult cutSinglePieceBatch(const Cutting::Piece::PieceWithMaterial &piece,
//                          int &remainingLength, const SelectedRod &rod,
//                          const CuttingMachine &machine,
//                          int currentOpId,
//                          int rodId,
//                          double kerf_mm,
//                          QVector<Cutting::Piece::PieceWithMaterial> &groupVec);

// CutResult cutComboBatch(const QVector<Cutting::Piece::PieceWithMaterial> &combo,
//                    int &remainingLength,
//                    const SelectedRod &rod,
//                    const CuttingMachine &machine,
//                    int currentOpId,
//                    int rodId,
//                    double kerf_mm,
//QVector<Cutting::Piece::PieceWithMaterial> &groupVec);