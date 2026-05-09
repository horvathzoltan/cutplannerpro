#include "stockfitengine.h"
#include "../../../common/logger.h"
#include "../../../common/identifierutils.h"
#include "../../../common/settingsmanager.h"

namespace Cutting {
namespace Optimizer {

std::optional<SelectedRod> StockFitEngine::pickStockRod(
    QVector<StockEntry>& stockInventory,
    const QSet<QUuid>& groupIds,
    int& rodCounter)
{
    zInfo(QString("🔍 STOCK SCAN START: groupIds=%1, stockCount=%2")
              .arg(groupIds.size())
              .arg(stockInventory.size()));

    for (int i = 0; i < stockInventory.size(); ++i) {
        StockEntry& stock = stockInventory[i];

        zInfo(QString("   • STOCK[%1]: materialId=%2, quantity=%3")
                  .arg(i)
                  .arg(stock.materialId.toString())
                  .arg(stock.quantity));

        if (!groupIds.contains(stock.materialId)) {
            zInfo(QString("     ⛔ SKIP STOCK[%1]: materialId not in groupIds").arg(i));
            continue;
        }
        if (stock.quantity <= 0) {
            zInfo(QString("     ⛔ SKIP STOCK[%1]: quantity=0").arg(i));
            continue;
        }

        zInfo(QString("     ✅ STOCK[%1] SELECTED: materialId=%2, newQuantity=%3")
                  .arg(i)
                  .arg(stock.materialId.toString())
                  .arg(stock.quantity - 1));

        stock.quantity--;

        SelectedRod rod;
        rod.materialId = stock.materialId;
        rod.length     = stock.master() ? stock.master()->stockLength_mm : 0;
        rod.isReusable = false;

        int matId = SettingsManager::instance().nextMaterialCounter();
        rod.barcode = IdentifierUtils::makeMaterialId(matId);

        rod.rodId = IdentifierUtils::makeRodId(++rodCounter);

        zInfo(QString("NEW STOCK ROD: rodCounter=%1, rodId=%2, materialId=%3, barcode=%4")
                  .arg(rodCounter)
                  .arg(rod.rodId)
                  .arg(rod.materialId.toString())
                  .arg(rod.barcode));

        return rod;
    }

    return std::nullopt;
}

} // namespace Optimizer
} // namespace Cutting


// zInfo("♻️ No reusable leftover fits — falling back to stock.");

// // 🧱 Ha nincs, akkor stockból - Stock vizsgálata — ANYAGCSOPORT ALAPÚ
// QSet<QUuid> groupIds = GroupUtils::groupMembers(
//     targetMaterialId); // már használod máshol is
// //for (auto &stock : _inventorySnapshot.profileInventory) {

// zInfo(QString("🔍 STOCK SCAN START: targetMaterialId=%1, stockCount=%2")
//           .arg(targetMaterialId.toString())
//           .arg(_inventorySnapshot.profileInventory.size()));

// for (int i = 0; i < _inventorySnapshot.profileInventory.size(); ++i) {
//     StockEntry &stock = _inventorySnapshot.profileInventory[i];

//     zInfo(QString("   • STOCK[%1]: materialId=%2, quantity=%3")
//               .arg(i)
//               .arg(stock.materialId.toString())
//               .arg(stock.quantity));


//     if (!groupIds.contains(stock.materialId)) {
//         zInfo(QString("     ⛔ SKIP STOCK[%1]: materialId not in groupIds").arg(i));
//         continue;
//     }
//     if (stock.quantity <= 0) {
//         zInfo(QString("     ⛔ SKIP STOCK[%1]: quantity=0").arg(i));
//         continue;
//     }

//     zInfo(QString("     ✅ STOCK[%1] SELECTED: materialId=%2, newQuantity=%3")
//               .arg(i)
//               .arg(stock.materialId.toString())
//               .arg(stock.quantity - 1));

//     stock.quantity--;
//     rod.materialId = stock.materialId; // ← lehet MÁS, mint targetMaterialId, de
//     // csoporttag
//     rod.length = stock.master() ? stock.master()->stockLength_mm : 0;

//     rod.isReusable = false;

//     // 🔑 Új, mesterséges stock barcode generálása
//     int matId = SettingsManager::instance().nextMaterialCounter();
//     rod.barcode = IdentifierUtils::makeMaterialId(matId);

//     zInfo(QString("🆕 Assigned MAT barcode=%1 for stock materialId=%2")
//               .arg(rod.barcode)
//               .arg(rod.materialId.toString()));

//     // 🔑 Stabil emberi azonosító
//     rod.rodId = IdentifierUtils::makeRodId(++rodCounter);

//     zInfo(QString("NEW STOCK ROD: rodCounter=%1, rodId=%2, materialId=%3, barcode=%4")
//               .arg(rodCounter)
//               .arg(rod.rodId)
//               .arg(rod.materialId.toString())
//               .arg(rod.barcode));

//     remainingLength = rod.length;

//     // [ 15 mm front trim ]  ← ezt előrehozzuk
//     // [ -------- hasznos hossz -------- ]
//     // [ 70 mm min. hulló ]
//     // [ 15 mm gyári vég ]  ← ebből vesszük el a front trimet

//     remainingLength2 = rod.length
//                        - OptimizerConstants::END_TRIM_MM        // front trim
//                        - OptimizerConstants::END_TRIM_MM        // gyári vég
//                        - OptimizerConstants::MINIMUM_HULLO_MM   // mechanikai minimum
//                        - OptimizerUtils::roundKerfLoss(1, kerf_mm);

//     rodSelected = true;

//     // stock ág végén
//     zInfo(QString("SELECTED STOCK ROD: rodId=%1, barcode=%2, length=%3")
//               .arg(rod.rodId)
//               .arg(rod.barcode)
//               .arg(rod.length));

//     break;
// }