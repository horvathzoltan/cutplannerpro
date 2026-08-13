#include "fitengine.h"
#include "stockfitengine.h"
#include "../../../common/logger.h"
#include "../../../common/identifierutils.h"
#include "service/cutting/optimizer/optimizerutils.h"
#include "settings/settingsmanager.h"

#include <materials/registry/material_registry.h>

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

// std::optional<SelectedRod> StockFitEngine::pickStockRod(
//     QVector<StockEntry>& stockInventory,
//     const QSet<QUuid>& groupIds,
//     int& rodCounter,
//     const QVector<Cutting::Piece::PieceWithMaterial>& pendingPieces,
//     double kerf_mm)
// {
//     int bestScore = std::numeric_limits<int>::min();
//     std::optional<SelectedRod> bestRod;

//     for (int i = 0; i < stockInventory.size(); ++i) {
//         StockEntry& stock = stockInventory[i];

//         // 1️⃣ Anyagcsoport + készlet szűrés
//         if (!groupIds.contains(stock.materialId))
//             continue;
//         if (stock.quantity <= 0)
//             continue;

//         const MaterialMaster* mat =
//             MaterialRegistry::instance().findById(stock.materialId);
//         MaterialScoringParams sp = mat ? mat->scoringParams()
//                                        : MaterialScoringParams::getDefault();

//         int rodLength = stock.master() ? stock.master()->stockLength_mm : 0;

//         // 2️⃣ dpLimit ehhez a szálhoz (ugyanaz a formula, mint initRodForMaterial-ben)
//         MaterialTrimmingParams tp = mat ? mat->trimmingParams(false)
//                                         : MaterialTrimmingParams::getDefault();

//         int dpLimitForRod = rodLength
//                             - tp.frontTrim_mm
//                             - tp.backTrim_mm
//                             - tp.minLeftOver_mm;

//         if (dpLimitForRod <= 0)
//             continue;

//         // 3️⃣ FitEngine futtatása erre a szálra
//         FitEngine::FitResult fr =
//             FitEngine::findBestFit(pendingPieces, dpLimitForRod, kerf_mm, sp);

//         if (fr.combo.isEmpty())
//             continue;

//         // 4️⃣ Scoring kiszámítása a meglévő calcScore() alapján
//         int usedNoKerf     = static_cast<int>(fr.used);   // darabhossz kerf nélkül
//         int waste          = dpLimitForRod - usedNoKerf;  // DP-limithez viszonyított hulladék
//         int leftoverLength = rodLength - usedNoKerf;      // fizikai leftover

//         int score = OptimizerUtils::calcScore(fr.pieceCount,
//                                               waste,
//                                               leftoverLength,
//                                               sp);

//         if (score > bestScore) {
//             bestScore = score;

//             SelectedRod rod;
//             rod.materialId = stock.materialId;
//             rod.length     = rodLength;
//             rod.isReusable = false;

//             int matCounter = SettingsManager::instance().nextMaterialCounter();
//             rod.barcode = IdentifierUtils::makeMaterialId(matCounter);
//             rod.rodId   = IdentifierUtils::makeRodId(rodCounter + 1);

//             bestRod = rod;
//         }
//     }

//     if (bestRod.has_value()) {
//         // 5️⃣ Készlet frissítése: quantity-- a kiválasztott szálra
//         for (auto& stock : stockInventory) {
//             if (stock.materialId == bestRod->materialId &&
//                 stock.master() &&
//                 stock.master()->stockLength_mm == bestRod->length)
//             {
//                 stock.quantity--;
//                 break;
//             }
//         }
//         rodCounter++;
//         return bestRod;
//     }

//     return std::nullopt;
// }


std::optional<SelectedRod> StockFitEngine::pickStockRod(
    QVector<StockEntry>& stockInventory,
    const QSet<QUuid>& groupIds,
    int& rodCounter)
{

    //zInfo("🔍 STOCK RÚD KERESÉSE — keresés indítása");
    int skipWrongGroup = 0;
    int skipZeroQty = 0;
    int total = stockInventory.size();
    int selectedIndex = -1;

    for (int i = 0; i < stockInventory.size(); ++i) {
        StockEntry& stock = stockInventory[i];

        if (!groupIds.contains(stock.materialId)) {
            //zInfo(QString("   ✖ Elutasítva: STOCK[%1] — rossz anyagcsoport").arg(i));
            skipWrongGroup++;
            continue;
        }
        if (stock.quantity <= 0) {
            //zInfo(QString("   ✖ Elutasítva: STOCK[%1] — nincs készlet (qty=0)").arg(i));
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

std::optional<SelectedRod> StockFitEngine::pickStockRod2(
    QVector<StockEntry>& stockInventory,
    const QSet<QUuid>& groupIds,
    int& rodCounter,
    int requestedLength_mm,
    double kerf_mm)
{
    struct Candidate {
        StockEntry* entry;
        int rodLength;
        int usableLength;
        int leftover;
        int score;
    };

    QVector<Candidate> candidates;

    for (auto& stock : stockInventory) {

        if (!groupIds.contains(stock.materialId))
            continue;

        if (stock.quantity <= 0)
            continue;

        const MaterialMaster* mat =
            MaterialRegistry::instance().findById(stock.materialId);
        if (!mat)
            continue;

        // trimming: front/back/minLeftOver
        MaterialTrimmingParams tp = mat->trimmingParams(false);

        // scoring: scrap / goodLeftOverMin / goodLeftOverMax
        MaterialScoringParams sp = mat->scoringParams();

        int rodLength = static_cast<int>(mat->stockLength_mm);

        int usableLength = rodLength
                           - tp.frontTrim_mm
                           - tp.backTrim_mm
                           - tp.minLeftOver_mm;

        int needed = requestedLength_mm + static_cast<int>(kerf_mm);

        if (usableLength < needed)
            continue;

        int leftover = rodLength - needed;

        int score = 0;
        if (leftover >= sp.goodLeftOver_Min_mm &&
            leftover <= sp.goodLeftOver_Max_mm)
        {
            score = 1000 - leftover;
        }
        else if (leftover < sp.scrap_mm) {
            score = -10000;
        }
        else {
            score = 100 - leftover;
        }

        zInfo(QString("   • STOCK jelölt: %1, rodLen=%2, usable=%3, leftover=%4, score=%5")
                  .arg(mat->barcode)
                  .arg(rodLength)
                  .arg(usableLength)
                  .arg(leftover)
                  .arg(score));

        candidates.append({ &stock, rodLength, usableLength, leftover, score });
    }

    if (candidates.isEmpty()){
        zInfo("   ✖ pickStockRod2 — nincs egyetlen trimming-kompatibilis jelölt sem");
        return std::nullopt;
    }

    std::sort(candidates.begin(), candidates.end(),
              [](const Candidate& a, const Candidate& b){
                  return a.score > b.score;
              });

    Candidate best = candidates.first();

    // --- ANYAGCSOPORT VÉDELEM A LEGJOBB STOCK JELÖLTRE ---
    if (!groupIds.contains(best.entry->materialId)) {
        zWarning("StockFitEngine: best-candidate rossz anyagcsoportból → tiltva");
        return std::nullopt;
    }


    best.entry->quantity--;

    SelectedRod rod;
    rod.materialId = best.entry->materialId;
    rod.length     = best.rodLength;
    rod.isReusable = false;

    int matCounter = SettingsManager::instance().nextMaterialCounter();
    rod.barcode = IdentifierUtils::makeMaterialId(matCounter);
    rod.rodId   = IdentifierUtils::makeRodId(++rodCounter);

    return rod;
}



} // namespace Optimizer
} // namespace Cutting


