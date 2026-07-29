// FILE: model/cutting/optimizer/toldasengine.h
#pragma once

#include "model/cutting/plan/request.h"
#include "model/cutting/piece/piecewithmaterial.h"
#include "materials/registry/material_registry.h"
#include "product/material_role_utils.h"
#include "model/inventorysnapshot.h"
#include "common/logger.h"

namespace Cutting {
namespace Optimizer {

// NEW: ToldasEngine - speciális súly toldás kezelő
class ToldasEngine
{
public:
    // outPieces: ide kerülnek a darabok (fődarab + toldat vagy csak fődarab)
    // outHandled: true, ha a requestet a ToldasEngine megoldotta (toldással vagy egy darabbal)
    static void computeToldasPieces(const Cutting::Plan::Request& req,
                                    const InventorySnapshot& inv,
                                    QVector<Cutting::Piece::PieceWithMaterial>& outPieces,
                                    bool& outHandled)
    {
        outPieces.clear();
        outHandled = false;

        // 1) MaterialMaster lekérdezése
        const MaterialMaster* mm =
            MaterialRegistry::instance().findById(req.materialId);

        if (!mm) {
            // Nincs anyag, nem tudunk mit tenni
            return;
        }

        // 2) Role meghatározása
        MaterialRole role =
            MaterialRoleUtils::makeRole(req, mm);

        const QString roleName = role.barcodePrefix;

        // 3) Csak NP-BAR (súly) esetén dolgozunk
        if (roleName != "NP-BAR") {
            // Nem súly, nem mi kezeljük
            return;
        }

        const int need = req.requiredLength;
        const int stock = mm->stockLength_mm;

        // 4) Leftoverek gyűjtése adott materialId-re
        QVector<LeftoverStockEntry> leftovers;
        for (const auto& l : inv.reusableInventory) {
            if (l.materialId == req.materialId)
                leftovers.append(l);
        }

        // 5) Toldás paraméterek (10 cm - 40 cm)
        const int minToldas_mm = 100;  // 10 cm
        const int maxToldas_mm = 400;  // 40 cm

        struct MainCandidate {
            bool fromStock;              // true: stock rúd, false: leftover
            int length_mm;               // fődarab hossza
            LeftoverStockEntry leftover; // csak akkor releváns, ha !fromStock
        };
        // PATCH 6 — FŐDARAB OPTIMALIZÁLÁS
        // Cél: a fődarab legyen leftoverből, minél nagyobb, de ne legyen túl nagy,
        // hogy a toldat (need - mainLen) beleessen a 100–400 mm tartományba.

        // 6) Fődarab jelöltek (optimalizált logika)

        // Először rendezzük a leftovereket nagy → kicsi sorrendbe
        std::sort(leftovers.begin(), leftovers.end(),
                  [](const LeftoverStockEntry& a, const LeftoverStockEntry& b) {
                      return a.availableLength_mm > b.availableLength_mm;
                  });


        // 6/a leftover fődarab jelöltek — csak azok kerülnek be,
        // PATCH 8 — FŐDARAB LEFTOVER OPTIMALIZÁLÁS
        // Cél: a fődarab legyen a leftoverek közül a "legjobb" jelölt.
        // A legjobb jelölt az, amely:
        //   - nem nagyobb a kért méretnél,
        //   - a toldat (need - mainLen) beleesik a 100–400 mm tartományba,
        //   - a toldat nem nagyobb, mint a fődarab fele,
        //   - a leftover waste minimális,
        //   - és a fődarab minél közelebb van a kért mérethez (need).

        // Új mainCandidates lista
        QVector<MainCandidate> mainCandidates;

        // A leftoverek már rendezve vannak nagy → kicsi sorrendbe (PATCH 6)
        int bestMainScore = std::numeric_limits<int>::max();
        MainCandidate bestMainCandidate;
        bool foundBestMain = false;

        for (const auto& l : leftovers) {

            int mainLen = l.availableLength_mm;

            // Nem lehet nagyobb a kért méretnél
            if (mainLen > need)
                continue;

            int deficit = need - mainLen;   // toldasLen

            // Toldat méretkorlátok
            if (deficit < minToldas_mm || deficit > maxToldas_mm)
                continue;

            // Toldat nem lehet nagyobb, mint a fődarab fele (PATCH 5)
            if (deficit > mainLen / 2)
                continue;

            // Waste minimalizálás: minél kisebb deficit → annál jobb
            // Score = deficit (minél kisebb, annál jobb)
            int score = deficit;

            if (score < bestMainScore) {
                bestMainScore = score;
                bestMainCandidate.fromStock = false;
                bestMainCandidate.length_mm = mainLen;
                bestMainCandidate.leftover = l;
                foundBestMain = true;
            }
        }

        // Ha találtunk ideális leftover fődarabot → csak azt tesszük be
        if (foundBestMain) {
            mainCandidates.append(bestMainCandidate);
        }


        // 6/b STOCK fődarab jelölt — engedjük a kicsit hosszabb stock rudat is (max +100 mm)
        {
            int mainLen = stock;

            if (mainLen > 0) {

                int deficit = need - mainLen;

                // Ha hosszabb a fődarab, de max 100 mm-rel → elfogadjuk
                if (deficit <= 0 && -deficit <= 100) {
                    MainCandidate c;
                    c.fromStock = true;
                    c.length_mm = mainLen;
                    mainCandidates.append(c);
                }

                // Ha rövidebb → toldható tartományban kell lennie
                if (deficit > 0 &&
                    deficit >= minToldas_mm &&
                    deficit <= maxToldas_mm &&
                    deficit <= mainLen / 2)
                {
                    MainCandidate c;
                    c.fromStock = true;
                    c.length_mm = mainLen;
                    mainCandidates.append(c);
                }
            }
        }


        // PATCH 6 — ha nincs egyetlen megfelelő fődarab jelölt sem,
        // akkor a régi fallback logika fogja kezelni később.


        // 7) Toldás keresése
        for (const auto& main : mainCandidates) {

            const int mainLen = main.length_mm;

            // 7/a Nem lehet nagyobb a fődarab, mint a kért méret
            if (mainLen > need) {
                // PATCH #4 — fődarab túl hosszú eset
                // Súlynál nem lehet hosszabb a fődarab, mert a záró dugó nem fér be.
                // Ezért ezt a jelöltet elvetjük.

                continue;
            }

            const int deficit = need - mainLen;

            // 7/b Ha deficit <= 0, akkor a fődarab önmagában elég vagy kicsit rövidebb
            if (deficit <= 0) {
                // Ha kicsit rövidebb, de max 10 cm, akkor elfogadjuk
                if (-deficit <= 100) {
                    Cutting::Piece::PieceInfo info;
                    info.length_mm = mainLen;
                    info.requestId = req.requestId;
                    info.externalReference = req.externalReference;

                    outPieces.append(Cutting::Piece::PieceWithMaterial(info, req.materialId));
                    outHandled = true;
                    return;
                }

                // Ha nagyobb az eltérés, nem jó jelölt
                continue;
            }

            // 7/c Toldat hossza (pozitív deficit)
            const int toldasLen = deficit;

            // Toldat hossz korlátok
            // PATCH #3 — toldat méretkorlátok finomítása
            // Ha a fődarab kicsit rövidebb (max 10 cm), akkor toldat nem kell.
            // Ha a deficit > 40 cm, akkor nem toldható.

            if (toldasLen < minToldas_mm) {
                // túl kicsi toldat → nem jó
                continue;
            }

            if (toldasLen > maxToldas_mm) {
                // túl nagy toldat → nem jó
                continue;
            }

            // PATCH #5 — szerelési szabály: a toldat nem lehet nagyobb, mint a fődarab fele
            // Ez azért kell, mert két nagy darabot nehéz összeragasztani, mozgatni.
            // A toldat legyen mindig lényegesen kisebb, mint a fődarab.
            if (toldasLen > mainLen / 2) {
                // túl nagy arányú toldat → nem szerelhető jól
                continue;
            }


            // 7/d Toldat forrás keresése (először leftover, aztán stock)
            bool foundToldas = false;
            bool toldasFromStock = false;
            LeftoverStockEntry toldasLeftover;
            QVector<LeftoverStockEntry> sortedToldasLeftovers = leftovers;

            // std::sort működik QVector-rel, mert begin()/end() STL kompatibilis iterátorokat ad.
            std::sort(sortedToldasLeftovers.begin(),
                      sortedToldasLeftovers.end(),
                      [](const LeftoverStockEntry& a, const LeftoverStockEntry& b) {
                          // toldat legyen minél kisebb → előnyben a rövidebb leftover
                          return a.availableLength_mm < b.availableLength_mm;
                      });
            // 7/d/1 leftover toldat jelöltek
            // PATCH 7 — toldat optimalizálás (waste minimalizálás + minél kisebb leftover toldat)
            //
            // Cél:
            //   - a toldat mindig a lehető legkisebb leftoverből jöjjön,
            //   - de csak akkor, ha az leftover elég hosszú,
            //   - és nem ugyanaz, mint a fődarab leftover,
            //   - és a maradék (waste) minél kisebb legyen.
            //
            // Megjegyzés:
            //   sortedToldasLeftovers már növekvő sorrendben van (kicsi → nagy),
            //   így a legkisebb megfelelő leftoveret fogjuk választani.

            int bestWaste = std::numeric_limits<int>::max();  // a lehető legkisebb waste-et keressük

            for (const auto& l : sortedToldasLeftovers) {

                // Nem használjuk ugyanazt a leftoveret fődarabnak és toldatnak
                if (!main.fromStock && l.entryId == main.leftover.entryId)
                    continue;

                // Csak olyan leftover jöhet szóba, amely elég hosszú a toldathoz
                if (l.availableLength_mm < toldasLen)
                    continue;

                // Waste = leftover teljes hossza - toldasLen
                int waste = l.availableLength_mm - toldasLen;

                // A legkisebb waste-et keressük → ez adja a legjobb toldatot
                if (waste < bestWaste) {
                    bestWaste = waste;
                    foundToldas = true;
                    toldasFromStock = false;
                    toldasLeftover = l;
                }
            }

            // Ha nincs leftover toldat, próbáljuk stockból
            if (!foundToldas && stock >= toldasLen) {
                foundToldas = true;
                toldasFromStock = true;
            }

            // 7/d/2 ha nincs leftover toldat, próbáljuk stockból
            if (!foundToldas && stock >= toldasLen) {
                foundToldas = true;
                toldasFromStock = true;
            }

            if (!foundToldas) {
                // Ehhez a fődarabhoz nem találtunk megfelelő toldatot
                continue;
            }

            // 7/e Megvan a fődarab + toldat kombináció
            // Fődarab: mainLen (nem vágjuk)
            // 7/e Megvan a fődarab + toldat kombináció
            // Fődarab: mainLen (nem vágjuk)
            // PATCH 9/a: fődarab jelölése az externalReference-ben
            Cutting::Piece::PieceInfo mainInfo;
            mainInfo.length_mm = mainLen;
            mainInfo.requestId = req.requestId;

            // PATCH 2 — leftover fődarab NEM VÁGANDÓ
            if (!main.fromStock) {
                mainInfo.keepWhole = true;                 // ← NEM VÁGJUK
                mainInfo.leftoverEntryId = main.leftover.entryId;  // ← jelöljük, hogy leftoverből jön
            } else {
                mainInfo.keepWhole = false;                // stock esetén továbbra is vágjuk
            }

            // Ha van externalReference, egészítsük ki egy jelöléssel.
            // Így a címkézés / cutting plan később felismerheti, hogy ez a fődarab.
            if (!req.externalReference.isEmpty())
                mainInfo.externalReference = req.externalReference + " [TOLDAS_MAIN]";
            else
                mainInfo.externalReference = "[TOLDAS_MAIN]";

            mainInfo.toldasRole = "TOLDAS_MAIN";

            outPieces.append(Cutting::Piece::PieceWithMaterial(mainInfo, req.materialId));

            // Toldat: toldasLen (ezt vágjuk)
            // PATCH 9/b: toldat jelölése az externalReference-ben
            Cutting::Piece::PieceInfo toldasInfo;
            toldasInfo.length_mm = toldasLen;
            toldasInfo.requestId = req.requestId;

            if (!req.externalReference.isEmpty())
                toldasInfo.externalReference = req.externalReference + " [TOLDAS_TOLDAT]";
            else
                toldasInfo.externalReference = "[TOLDAS_TOLDAT]";

            toldasInfo.toldasRole = "TOLDAS_TOLDAT";

            // PATCH 11/B — leftover eredet jelölése
            if (!toldasFromStock)
                toldasInfo.leftoverEntryId = toldasLeftover.entryId;

            outPieces.append(Cutting::Piece::PieceWithMaterial(toldasInfo, req.materialId));

            outHandled = true;
            return;
        }

        // 8) Ha idáig jutottunk, nem találtunk megfelelő toldást vagy elfogadható egy darabot
        // A normál vágási engine próbálkozhat tovább, vagy hibát dobhat.
        // PATCH #1: QUuid konverzió QString-re, mert arg() nem támogatja a QUuid típust
        QString reqIdStr = req.requestId.toString();

        zInfo(QString("ToldasEngine: nem találtunk megfelelő toldást a requestId=%1, need=%2, stock=%3")
                  .arg(reqIdStr)   // PATCH: konvertált QString-et adunk át
                  .arg(need)
                  .arg(stock));

    }
};

} // namespace Optimizer
} // namespace Cutting
