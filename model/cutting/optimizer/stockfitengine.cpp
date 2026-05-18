#include "stockfitengine.h"
#include "../../../common/logger.h"
#include "../../../common/identifierutils.h"
#include "../../../common/settingsmanager.h"

namespace Cutting {
namespace Optimizer {

bool StockFitEngine::_isVerbose = false;

/*
🧭 STOCK ROD KERESÉS — MIT CSINÁL EZ A FÜGGVÉNY?

Ez a függvény megkeresi, hogy a STOCK készletből van‑e olyan rúd,
amely:

  ✔ az adott anyagcsoportba tartozik
  ✔ van belőle készlet (quantity > 0)
  ✔ és még nem fogyott el

A keresés lépései:

1️⃣ Végigmegyünk az összes stock tételen (stockInventory)
2️⃣ Minden tételnél eldöntjük:
      • anyagcsoport egyezik?      → ha nem: skipWrongGroup++
      • quantity > 0?              → ha nem: skipZeroQty++
      • ha mindkettő OK → TALÁLAT
3️⃣ Az első megfelelő tételt kiválasztjuk:
      • quantity--
      • új rodId generálása
      • új barcode generálása
      • SelectedRod visszaadása
4️⃣ Ha nem találtunk semmit:
      • visszatérünk std::nullopt‑tal
      • és logoljuk a keresés összegzését

Röviden:
Ez a “stock‑vadász”, amely megmondja,
hogy van‑e még használható gyári szál az adott anyaghoz.
*/

std::optional<SelectedRod> StockFitEngine::pickStockRod(
    QVector<StockEntry>& stockInventory,
    const QSet<QUuid>& groupIds,
    int& rodCounter)
{
    zInfo("🔍 STOCK RÚD KERESÉSE — keresés indítása");
    int skipWrongGroup = 0;
    int skipZeroQty = 0;
    int total = stockInventory.size();
    int selectedIndex = -1;

    for (int i = 0; i < stockInventory.size(); ++i) {
        StockEntry& stock = stockInventory[i];

        if (!groupIds.contains(stock.materialId)) {
            zInfo(QString("   ✖ Elutasítva: STOCK[%1] — rossz anyagcsoport").arg(i));
            skipWrongGroup++;
            continue;
        }
        if (stock.quantity <= 0) {
            zInfo(QString("   ✖ Elutasítva: STOCK[%1] — nincs készlet (qty=0)").arg(i));
            skipZeroQty++;
            continue;
        }

        zInfo(QString("   ✔ Vizsgálat OK: STOCK[%1] — anyag és készlet rendben").arg(i));
        selectedIndex = i;

        stock.quantity--;

        SelectedRod rod;
        rod.materialId = stock.materialId;
        rod.length     = stock.master() ? stock.master()->stockLength_mm : 0;
        rod.isReusable = false;

        int matId = SettingsManager::instance().nextMaterialCounter();
        rod.barcode = IdentifierUtils::makeMaterialId(matId);
        rod.rodId = IdentifierUtils::makeRodId(++rodCounter);

        zInfo(QString("✔ TALÁLAT: STOCK[%1] → új rúd kiválasztva (rodId=%2, barcode=%3, length=%4)")
                  .arg(i)
                  .arg(rod.rodId)
                  .arg(rod.barcode)
                  .arg(rod.length));

        zInfo(QString("   → Készlet frissítve: material=%1, newQuantity=%2")
                  .arg(stock.materialBarcode())
                  .arg(stock.quantity));

        // AGGREGÁLT ÖSSZEFOGLALÓ
        // zInfo(QString("📊 STOCK KERESÉS ÖSSZEGZÉS: total=%1, találat=0, skipWrongGroup=%2, skipZeroQty=%3")
        //           .arg(total)
        //           .arg(skipWrongGroup)
        //           .arg(skipZeroQty));


        return rod;
    }

    // NEM TALÁLT SEMMIT → összegzés
    zInfo(QString("📊 STOCK KERESÉS ÖSSZEGZÉS: total=%1, találat=0, skipWrongGroup=%2, skipZeroQty=%3")
              .arg(total)
              .arg(skipWrongGroup)
              .arg(skipZeroQty));


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