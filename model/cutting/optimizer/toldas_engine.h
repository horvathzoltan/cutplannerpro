// FILE: model/cutting/optimizer/toldasengine.h
#pragma once

#include "model/cutting/plan/request.h"
#include "model/cutting/piece/piecewithmaterial.h"
#include "materials/registry/material_registry.h"
#include "product/utils/material_role_utils.h"
#include "model/inventorysnapshot.h"
#include "common/logger.h"

#include "leftover/registry/leftoverstockregistry.h"

#include <settings/settingsmanager.h>

namespace Cutting {
namespace Optimizer {

// NEW: ToldasEngine - speciális súly toldás kezelő
class ToldasEngine
{
public:
    static bool isStockMainCandidate(int need,
                              int stockLen,
                              bool noLeftoverMain,
                              int maxMainOverlength,
                              int minToldas_mm,
                              int maxToldas_mm)
    {
        int deficit = need - stockLen;

        // 1) Ha nincs leftover fődarab → automatikusan jelölt
        if (noLeftoverMain)
            return true;

        // 2) Ha a fődarab hosszabb, de max túlhossz toleranciával → jelölt
        if (deficit <= 0 && -deficit <= maxMainOverlength)
            return true;

        // 3) Ha toldható tartományban van → jelölt
        if (deficit > 0 &&
            deficit >= minToldas_mm &&
            deficit <= maxToldas_mm &&
            deficit <= stockLen / 2)
            return true;

        return false;
    }

    static std::optional<LeftoverStockEntry>
    findBestToldasLeftover(const QVector<LeftoverStockEntry>& leftovers,
                           int toldasLen,
                           std::optional<QUuid> excludeEntryId)
    {
        int bestWaste = std::numeric_limits<int>::max();
        std::optional<LeftoverStockEntry> best;

        for (const auto& l : leftovers) {

            if (excludeEntryId.has_value() && l.entryId == excludeEntryId.value())
                continue;

            if (l.availableLength_mm < toldasLen)
                continue;

            int waste = l.availableLength_mm - toldasLen;

            if (waste < bestWaste) {
                bestWaste = waste;
                best = l;
            }
        }

        return best;
    }


    static bool handleSpecialCaseStockLargeToldas(
        const Cutting::Plan::Request& req,
        const InventorySnapshot& inv,
        const QVector<LeftoverStockEntry>& leftovers,
        int need,
        int stock,
        QVector<Cutting::Piece::PieceWithMaterial>& outPieces)
    {
        if (stock <= 0 || need <= stock)
            return false;

        int mainLen   = stock;
        int toldasLen = need - mainLen;

        // szerelési korlát
        if (toldasLen <= 0 || toldasLen > mainLen / 2)
            return false;

        // leftover toldat keresése
        auto bestLeftover = findBestToldasLeftover(leftovers, toldasLen, std::nullopt);

        bool toldasFromStock = false;
        LeftoverStockEntry toldasLeftover;

        if (bestLeftover.has_value()) {
            toldasLeftover = bestLeftover.value();
        } else {
            if (stock < toldasLen)
                return false;
            toldasFromStock = true;
        }

        // --- FŐDARAB ---
        Cutting::Piece::PieceInfo mainInfo;
        mainInfo.length_mm = mainLen;
        mainInfo.requestId = req.requestId;
        mainInfo.keepWhole = false;

        if (!req.externalReference.isEmpty())
            mainInfo.externalReference = req.externalReference + " [TOLDAS_MAIN]";
        else
            mainInfo.externalReference = "[TOLDAS_MAIN]";

        mainInfo.toldasRole = ToldasRole::Main;

        outPieces.append(Cutting::Piece::PieceWithMaterial(mainInfo, req.materialId));

        // --- TOLDAT ---
        Cutting::Piece::PieceInfo toldasInfo;
        toldasInfo.length_mm = toldasLen;
        toldasInfo.requestId = req.requestId;

        if (!req.externalReference.isEmpty())
            toldasInfo.externalReference = req.externalReference + " [TOLDAS_TOLDAT]";
        else
            toldasInfo.externalReference = "[TOLDAS_TOLDAT]";

        toldasInfo.toldasRole = ToldasRole::Toldat;

        if (!toldasFromStock)
            toldasInfo.leftoverEntryId = toldasLeftover.entryId;

        outPieces.append(Cutting::Piece::PieceWithMaterial(toldasInfo, req.materialId));

        return true;
    }





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

        // --- Toldás paraméterek betöltése settingsből ---
        auto& sm = SettingsManager::instance();
        const int SAFETY_MARGIN_MM =sm.safetyMargin();
        const int MAX_MAIN_SHORTFALL_MM =sm.maxMainShortfall();
        // Hard-limit paraméterek (nem jelennek meg a GUI-ban)
        const int minToldas_mm =sm.toldasMin();
        const int maxToldas_mm = sm.toldasMax();
        const int maxMainOverlength = sm.maxMainOverlength();
        const int need = req.requiredLength;
        const int stock = mm->stockLength_mm;

        // --- VALÓDI STOCK-ELLENŐRZÉS VASTAG SÚLYRA ---
        bool hasThickStock = false;
        bool hasThickLeftover = false;

        // teljes rudak (stock)
        for (const auto& s : inv.profileInventory) {
            if (s.materialId == req.materialId) {
                hasThickStock = true;
                break;
            }
        }

        // leftover rudak
        for (const auto& l : inv.reusableInventory) {
            if (l.materialId == req.materialId) {
                hasThickLeftover = true;
                break;
            }
        }

        // ❗ Ha nincs vastag súly se stockban, se leftoverben → NEM kezeljük toldással
        if (!hasThickStock && !hasThickLeftover) {
            zInfo(QString("ToldasEngine: nincs vastag súly készlet → vékony súly fallback"));
            return;   // handled = false → PieceBuilderToldas fallback vékony súlyra
        }

        // 4) Leftoverek gyűjtése adott materialId-re
        QVector<LeftoverStockEntry> leftovers;
        for (const auto& l : inv.reusableInventory) {
            if (l.materialId == req.materialId)
                leftovers.append(l);
        }

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


        // // 6/b STOCK fődarab jelölt — engedjük a kicsit hosszabb stock rudat is (max +100 mm)
        // {
        //     int mainLen = stock;

        //     if (mainLen > 0) {

        //         int deficit = need - mainLen;

        //         // Ha hosszabb a fődarab, de max 100 mm-rel → elfogadjuk
        //         if (deficit <= 0 && -deficit <= maxMainOverlength) {
        //             MainCandidate c;
        //             c.fromStock = true;
        //             c.length_mm = mainLen;
        //             mainCandidates.append(c);
        //         }

        //         // PATCH #STOCK_TOLDAS_OVERRIDE — Engedjük a nagy toldatot, ha nincs leftover fődarab
        //         // Ha nincs leftover fődarab jelölt, akkor a stock fődarab + nagy toldat is engedett.
        //         // Ez kell a 3835 mm-es súlyhoz (3000 + 835 mm).

        //         bool noLeftoverMain = !foundBestMain;   // ha nincs leftover fődarab

        //         // 6/b STOCK fődarab jelölt — egyszerű, determinisztikus logika
        //         {
        //             int mainLen = stock;

        //             if (mainLen > 0) {

        //                 int deficit = need - mainLen;

        //                 bool noLeftoverMain = !foundBestMain;

        //                 // 6/b/1 — Ha nincs leftover fődarab → a stock fődarab automatikusan jelölt
        //                 if (noLeftoverMain) {
        //                     MainCandidate c;
        //                     c.fromStock = true;
        //                     c.length_mm = mainLen;
        //                     mainCandidates.append(c);
        //                 }

        //                 // 6/b/2 — Ha van leftover fődarab → csak a normál korlátok szerint engedjük
        //                 if (!noLeftoverMain) {

        //                     // fődarab hosszabb, de max 100 mm-rel → elfogadjuk
        //                     if (deficit <= 0 && -deficit <= maxMainOverlength) {
        //                         MainCandidate c;
        //                         c.fromStock = true;
        //                         c.length_mm = mainLen;
        //                         mainCandidates.append(c);
        //                     }

        //                     // toldható tartomány (100–400 mm + fele szabály)
        //                     if (deficit > 0 &&
        //                         deficit >= minToldas_mm &&
        //                         deficit <= maxToldas_mm &&
        //                         deficit <= mainLen / 2)
        //                     {
        //                         MainCandidate c;
        //                         c.fromStock = true;
        //                         c.length_mm = mainLen;
        //                         mainCandidates.append(c);
        //                     }
        //                 }
        //             }
        //         }



        //         // // Ha rövidebb → toldható tartományban kell lennie
        //         // if (deficit > 0 &&
        //         //     deficit >= minToldas_mm &&
        //         //     deficit <= maxToldas_mm &&
        //         //     deficit <= mainLen / 2)
        //         // {
        //         //     MainCandidate c;
        //         //     c.fromStock = true;
        //         //     c.length_mm = mainLen;
        //         //     mainCandidates.append(c);
        //         // }
        //     }
        // } // endof 6B
        {
            int mainLen = stock;

            if (mainLen > 0) {

                bool noLeftoverMain = !foundBestMain;

                if (isStockMainCandidate(need,
                                         mainLen,
                                         noLeftoverMain,
                                         maxMainOverlength,
                                         minToldas_mm,
                                         maxToldas_mm))
                {
                    MainCandidate c;
                    c.fromStock = true;
                    c.length_mm = mainLen;

                    mainCandidates.append(c);
                }
            }
        }

        // SPECIAL CASE: nem vágható NP-BAR súly (need > stock)
        // Ilyenkor a ToldasEngine alapfeladata, hogy megoldja a súlyt:
        //   - fődarab: stock rúd (mm->stockLength_mm),
        //   - toldat: need - stock, akár nagyobb, mint a "normál" 100–400 mm tartomány,
        //   - egyetlen szerelési korlát: a toldat nem lehet nagyobb, mint a fődarab fele.
        //
        // Ha nincs leftover-alapú fődarab jelölt (mainCandidates üres),
        // de a need > stock, akkor itt kényszerítjük a stock+large-toldat megoldást.
        //if (mainCandidates.isEmpty() && stock > 0 && need > stock) {
        // if (!foundBestMain && stock > 0 && need > stock) {

        //     const int mainLen   = stock;
        //     const int toldasLen = need - mainLen;

        //     // Toldatnak pozitívnak kell lennie, és nem lehet nagyobb, mint a fődarab fele
        //     if (toldasLen > 0 && toldasLen <= mainLen / 2) {

        //         bool foundToldas     = false;
        //         bool toldasFromStock = false;
        //         LeftoverStockEntry toldasLeftover;

        //         // Toldat forrás keresése (először leftover, aztán stock),
        //         // ugyanazzal a waste-minimalizáló logikával, mint a 7/d-ben,
        //         // csak itt NEM korlátozzuk 100–400 mm közé.
        //         QVector<LeftoverStockEntry> sortedToldasLeftovers = leftovers;

        //         std::sort(sortedToldasLeftovers.begin(),
        //                   sortedToldasLeftovers.end(),
        //                   [](const LeftoverStockEntry& a, const LeftoverStockEntry& b) {
        //                       // toldat legyen minél kisebb → előnyben a rövidebb leftover
        //                       return a.availableLength_mm < b.availableLength_mm;
        //                   });

        //         int bestWaste = std::numeric_limits<int>::max();

        //         for (const auto& l : sortedToldasLeftovers) {

        //             // Csak olyan leftover jöhet szóba, amely elég hosszú a toldathoz
        //             if (l.availableLength_mm < toldasLen)
        //                 continue;

        //             int waste = l.availableLength_mm - toldasLen;

        //             if (waste < bestWaste) {
        //                 bestWaste       = waste;
        //                 foundToldas     = true;
        //                 toldasFromStock = false;
        //                 toldasLeftover  = l;
        //             }
        //         }

        //         // Ha nincs leftover toldat, próbáljuk stockból
        //         if (!foundToldas && stock >= toldasLen) {
        //             foundToldas     = true;
        //             toldasFromStock = true;
        //         }

        //         if (foundToldas) {
        //             // Fődarab: stock rúd
        //             Cutting::Piece::PieceInfo mainInfo;
        //             mainInfo.length_mm = mainLen;
        //             mainInfo.requestId = req.requestId;
        //             mainInfo.keepWhole = false; // stockból vágjuk

        //             if (!req.externalReference.isEmpty())
        //                 mainInfo.externalReference = req.externalReference + " [TOLDAS_MAIN]";
        //             else
        //                 mainInfo.externalReference = "[TOLDAS_MAIN]";

        //             mainInfo.toldasRole = "TOLDAS_MAIN";

        //             outPieces.append(Cutting::Piece::PieceWithMaterial(mainInfo, req.materialId));

        //             // Toldat
        //             Cutting::Piece::PieceInfo toldasInfo;
        //             toldasInfo.length_mm = toldasLen;
        //             toldasInfo.requestId = req.requestId;

        //             if (!req.externalReference.isEmpty())
        //                 toldasInfo.externalReference = req.externalReference + " [TOLDAS_TOLDAT]";
        //             else
        //                 toldasInfo.externalReference = "[TOLDAS_TOLDAT]";

        //             toldasInfo.toldasRole = "TOLDAS_TOLDAT";

        //             if (!toldasFromStock)
        //                 toldasInfo.leftoverEntryId = toldasLeftover.entryId;

        //             outPieces.append(Cutting::Piece::PieceWithMaterial(toldasInfo, req.materialId));

        //             zInfo(QString("ToldasEngine: SPECIAL non-cuttable NP-BAR → stock MAIN + large TOLDAT, need=%1, main=%2, toldat=%3")
        //                       .arg(need)
        //                       .arg(mainLen)
        //                       .arg(toldasLen));

        //             for (const auto& p : outPieces) {
        //                 auto l = LeftoverStockRegistry::instance().findById(*p.info.leftoverEntryId);
        //                 QString ltxt = l.has_value() ? (l->barcode) : "?";

        //                 zInfo(QString("  → piece: len=%1, role=%2, leftover=%3")
        //                           .arg(p.info.length_mm)
        //                           .arg(p.info.toldasRole)
        //                           .arg(ltxt));
        //             }

        //             outHandled = true;
        //             return;
        //         }
        //     }
        // } // endof SpecialCase


        // SPECIAL CASE: need > stock és nincs leftover fődarab
        if (!foundBestMain) {
            if (handleSpecialCaseStockLargeToldas(req, inv, leftovers, need, stock, outPieces)) {
                outHandled = true;
                return;
            }
        }



        // 7) Toldás keresése
        for (const auto& main : mainCandidates) {

            const int mainLen = main.length_mm;
            const int deficit  = need - mainLen;

            // 7/a Nem lehet nagyobb a fődarab, mint a kért méret
            if (mainLen > need) {
                // Súlynál nem lehet hosszabb a fődarab, mert a záró dugó nem fér be.
                // Ezért ezt a jelöltet elvetjük.
                continue;
            }


            // 7/b Ha deficit <= 0, akkor a fődarab önmagában elég vagy kicsit rövidebb
            if (deficit <= 0) {
                // Ha kicsit rövidebb, de max 10 cm, akkor elfogadjuk
                if (-deficit <= MAX_MAIN_SHORTFALL_MM) {
                    Cutting::Piece::PieceInfo info;
                    info.length_mm = mainLen;
                    info.requestId = req.requestId;
                    info.externalReference = req.externalReference;

                    outPieces.append(Cutting::Piece::PieceWithMaterial(info, req.materialId));

                    zInfo(QString("ToldasEngine: RETURNING %1 pieces, handled=%2")
                              .arg(outPieces.size())
                              .arg(outHandled));
                    for (const auto& p : outPieces) {
                        auto l = LeftoverStockRegistry::instance().findById(*p.info.leftoverEntryId);
                        QString ltxt = l.has_value()?(l->barcode):"?";


                        zInfo(QString("  → piece: len=%1, role=%2, leftover=%3")
                                  .arg(p.info.length_mm)
                                  .arg(ToldasRoleUtils::toDisplayText(p.info.toldasRole))
                                  .arg(ltxt));
                    }
                    outHandled = true;
                    return;
                }

                // Ha nagyobb az eltérés, nem jó jelölt
                continue;
            }

            // 7/c Toldat hossza (pozitív deficit)
            int toldasLen = deficit;

            // PATCH — toldat csökkentése a biztonsági ráhagyás miatt
            toldasLen -= SAFETY_MARGIN_MM;
            // if (toldasLen < minToldas_mm)
            //     toldasLen = minToldas_mm;   // ne essen a minimum alá

            // Toldat hossz korlátok

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


            // // 7/d Toldat forrás keresése (először leftover, aztán stock)
            // bool foundToldas = false;
            // bool toldasFromStock = false;
            // LeftoverStockEntry toldasLeftover;

            // QVector<LeftoverStockEntry> sortedToldasLeftovers = leftovers;

            // std::sort(sortedToldasLeftovers.begin(),
            //           sortedToldasLeftovers.end(),
            //           [](const LeftoverStockEntry& a, const LeftoverStockEntry& b) {
            //               // toldat legyen minél kisebb → előnyben a rövidebb leftover
            //               return a.availableLength_mm < b.availableLength_mm;
            //           });

            // // 7/d/1 leftover toldat jelöltek
            // // PATCH 7 — toldat optimalizálás (waste minimalizálás + minél kisebb leftover toldat)
            // //
            // // Cél:
            // //   - a toldat mindig a lehető legkisebb leftoverből jöjjön,
            // //   - de csak akkor, ha az leftover elég hosszú,
            // //   - és nem ugyanaz, mint a fődarab leftover,
            // //   - és a maradék (waste) minél kisebb legyen.
            // //
            // // Megjegyzés:
            // //   sortedToldasLeftovers már növekvő sorrendben van (kicsi → nagy),
            // //   így a legkisebb megfelelő leftoveret fogjuk választani.

            // int bestWaste = std::numeric_limits<int>::max();  // a lehető legkisebb waste-et keressük

            // for (const auto& l : sortedToldasLeftovers) {

            //     // Nem használjuk ugyanazt a leftoveret fődarabnak és toldatnak
            //     if (!main.fromStock && l.entryId == main.leftover.entryId)
            //         continue;

            //     // Csak olyan leftover jöhet szóba, amely elég hosszú a toldathoz
            //     if (l.availableLength_mm < toldasLen)
            //         continue;

            //     int waste = l.availableLength_mm - toldasLen;

            //     if (waste < bestWaste) {
            //         bestWaste       = waste;
            //         foundToldas     = true;
            //         toldasFromStock = false;
            //         toldasLeftover  = l;
            //     }
            // }

            // // Ha nincs leftover toldat, próbáljuk stockból
            // if (!foundToldas && stock >= toldasLen) {
            //     foundToldas     = true;
            //     toldasFromStock = true;
            // }

            // if (!foundToldas) {
            //     // Ehhez a fődarabhoz nem találtunk megfelelő toldatot
            //     continue;
            // }

            // 7/d Toldat forrás keresése (először leftover, aztán stock)
            bool foundToldas     = false;
            bool toldasFromStock = false;
            LeftoverStockEntry toldasLeftover;

            // PATCH 7 — toldat optimalizálás (waste minimalizálás + minél kisebb leftover toldat)
            //
            // Cél:
            //   - a toldat mindig a lehető legkisebb leftoverből jöjjön,
            //   - de csak akkor, ha az leftover elég hosszú,
            //   - és nem ugyanaz, mint a fődarab leftover,
            //   - és a maradék (waste) minél kisebb legyen.

            std::optional<QUuid> excludeId;
            if (!main.fromStock)
                excludeId = main.leftover.entryId;

            auto bestLeftover = findBestToldasLeftover(leftovers, toldasLen, excludeId);

            if (bestLeftover.has_value()) {
                foundToldas     = true;
                toldasFromStock = false;
                toldasLeftover  = bestLeftover.value();
            }

            // Ha nincs leftover toldat, próbáljuk stockból
            if (!foundToldas && stock >= toldasLen) {
                foundToldas     = true;
                toldasFromStock = true;
            }

            if (!foundToldas) {
                // Ehhez a fődarabhoz nem találtunk megfelelő toldatot
                continue;
            }

            // 7/e Megvan a fődarab + toldat kombináció

            // Fődarab
            Cutting::Piece::PieceInfo mainInfo;
            mainInfo.length_mm = mainLen;
            mainInfo.requestId = req.requestId;

            if (!main.fromStock) {
                mainInfo.keepWhole       = true;
                mainInfo.leftoverEntryId = main.leftover.entryId;
            } else {
                mainInfo.keepWhole = false;
            }

            if (!req.externalReference.isEmpty())
                mainInfo.externalReference = req.externalReference + " [TOLDAS_MAIN]";
            else
                mainInfo.externalReference = "[TOLDAS_MAIN]";

            mainInfo.toldasRole = ToldasRole::Main;

            outPieces.append(Cutting::Piece::PieceWithMaterial(mainInfo, req.materialId));

            // Toldat
            Cutting::Piece::PieceInfo toldasInfo;
            toldasInfo.length_mm   = toldasLen;
            toldasInfo.requestId   = req.requestId;

            if (!req.externalReference.isEmpty())
                toldasInfo.externalReference = req.externalReference + " [TOLDAS_TOLDAT]";
            else
                toldasInfo.externalReference = "[TOLDAS_TOLDAT]";

            toldasInfo.toldasRole = ToldasRole::Toldat;

            if (!toldasFromStock)
                toldasInfo.leftoverEntryId = toldasLeftover.entryId;

            outPieces.append(Cutting::Piece::PieceWithMaterial(toldasInfo, req.materialId));


            zInfo(QString("ToldasEngine: RETURNING %1 pieces, handled=%2")
                      .arg(outPieces.size())
                      .arg(outHandled));
            for (const auto& p : outPieces) {
                auto l = LeftoverStockRegistry::instance().findById(*p.info.leftoverEntryId);
                QString ltxt = l.has_value()?(l->barcode):"?";

                zInfo(QString("  → piece: len=%1, role=%2, leftover=%3")
                          .arg(p.info.length_mm)
                          .arg(ToldasRoleUtils::toDisplayText(p.info.toldasRole))
                          .arg(ltxt));
            }

            outHandled = true;
            return;
        } // end of 7

        // 8) Ha idáig jutottunk, nem találtunk megfelelő toldást vagy elfogadható egy darabot
        QString reqIdStr = req.requestId.toString();

        zInfo(QString("ToldasEngine: nem találtunk megfelelő toldást a requestId=%1, need=%2, stock=%3")
                  .arg(reqIdStr)
                  .arg(need)
                  .arg(stock));
    }


    static bool computeReplacementFromThinBars(
        const Cutting::Plan::Request& req,
        const InventorySnapshot& inv,
        QVector<Cutting::Piece::PieceWithMaterial>& outPieces)
    {
        outPieces.clear();

        const int need = req.requiredLength;

        // 1) Vékony súly anyag lekérése
        const MaterialMaster* thinMat =
            MaterialRegistry::instance().findByBarcode("NP-BAR12-4000");
        if (!thinMat)
            return false;

        const QUuid thinId   = thinMat->id;
        const int   stockLen = thinMat->stockLength_mm;

        // Toldás paraméterek
        auto& sm = SettingsManager::instance();
        const int minToldas_mm = sm.toldasMin();
        const int maxToldas_mm = sm.toldasMax();

        // 2) Vékony leftoverek gyűjtése
        QVector<LeftoverStockEntry> thinLeftovers;
        for (const auto& l : inv.reusableInventory) {
            if (l.materialId == thinId)
                thinLeftovers.append(l);
        }

        // --- 3) Egy darabos helyettesítés: STOCK ---
        if (stockLen >= need) {
            Cutting::Piece::PieceInfo info;
            info.length_mm        = need;
            info.requestId        = req.requestId;
            info.externalReference = req.externalReference + " [REPLACE_THIN]";
            info.toldasRole       = ToldasRole::None;
            info.keepWhole        = false;

            outPieces.append(Cutting::Piece::PieceWithMaterial(info, thinId));
            return true;
        }

        // --- 4) Egy darabos helyettesítés: LEFTOVER ---
        {
            int bestWaste = std::numeric_limits<int>::max();
            std::optional<LeftoverStockEntry> best;

            for (const auto& l : thinLeftovers) {
                if (l.availableLength_mm < need)
                    continue;

                int waste = l.availableLength_mm - need;
                if (waste < bestWaste) {
                    bestWaste = waste;
                    best      = l;
                }
            }

            if (best.has_value()) {
                const auto& lo = best.value();

                Cutting::Piece::PieceInfo info;
                info.length_mm        = need;
                info.requestId        = req.requestId;
                info.externalReference = req.externalReference + " [REPLACE_THIN]";
                info.toldasRole       = ToldasRole::None;
                info.keepWhole        = true;
                info.leftoverEntryId  = lo.entryId;

                outPieces.append(Cutting::Piece::PieceWithMaterial(info, thinId));
                return true;
            }
        }

        // --- 5) Két darabos helyettesítés (toldás) ---
        struct ThinMainCandidate {
            enum class Source { Stock, Leftover } source;
            int length_mm;
            LeftoverStockEntry leftover; // csak akkor releváns, ha leftover
        };

        QVector<ThinMainCandidate> mains;

        // 5/a STOCK fődarab jelölt
        if (stockLen > 0 && stockLen < need) {
            ThinMainCandidate c;
            c.source    = ThinMainCandidate::Source::Stock;
            c.length_mm = stockLen;
            mains.append(c);
        }

        // 5/b LEFTOVER fődarab jelöltek
        for (const auto& l : thinLeftovers) {
            if (l.availableLength_mm <= 0 || l.availableLength_mm >= need)
                continue;

            ThinMainCandidate c;
            c.source    = ThinMainCandidate::Source::Leftover;
            c.length_mm = l.availableLength_mm;
            c.leftover  = l;
            mains.append(c);
        }

        // 6) Fődarab + toldat kombináció keresése
        bool foundCombo = false;
        ThinMainCandidate bestMain;
        LeftoverStockEntry bestToldasLeftover;
        bool toldasFromStock = false;
        int bestTotalWaste   = std::numeric_limits<int>::max();

        for (const auto& main : mains) {

            const int mainLen = main.length_mm;
            int toldasLen     = need - mainLen;
            if (toldasLen <= 0)
                continue;

            // Toldat korlátok
            if (toldasLen < minToldas_mm || toldasLen > maxToldas_mm)
                continue;
            if (toldasLen > mainLen / 2)
                continue;

            // Toldat forrás keresése
            int bestWaste = std::numeric_limits<int>::max();
            std::optional<LeftoverStockEntry> bestLo;

            for (const auto& l : thinLeftovers) {

                // Nem használjuk ugyanazt leftoveret fődarabnak és toldatnak
                if (main.source == ThinMainCandidate::Source::Leftover &&
                    l.entryId == main.leftover.entryId)
                    continue;

                if (l.availableLength_mm < toldasLen)
                    continue;

                int waste = l.availableLength_mm - toldasLen;
                if (waste < bestWaste) {
                    bestWaste = waste;
                    bestLo    = l;
                }
            }

            bool localFoundToldas     = false;
            bool localToldasFromStock = false;
            LeftoverStockEntry localToldasLeftover;

            if (bestLo.has_value()) {
                localFoundToldas     = true;
                localToldasFromStock = false;
                localToldasLeftover  = bestLo.value();
            } else if (stockLen >= toldasLen) {
                localFoundToldas     = true;
                localToldasFromStock = true;
            }

            if (!localFoundToldas)
                continue;

            int mainWaste =
                (main.source == ThinMainCandidate::Source::Stock)
                    ? std::max(0, stockLen - mainLen)
                    : std::max(0, main.leftover.availableLength_mm - mainLen);

            int toldasWaste =
                localToldasFromStock
                    ? std::max(0, stockLen - toldasLen)
                    : std::max(0, localToldasLeftover.availableLength_mm - toldasLen);

            int totalWaste = mainWaste + toldasWaste;

            if (totalWaste < bestTotalWaste) {
                bestTotalWaste     = totalWaste;
                bestMain           = main;
                bestToldasLeftover = localToldasLeftover;
                toldasFromStock    = localToldasFromStock;
                foundCombo         = true;
            }
        }

        if (!foundCombo)
            return false;

        // 7) Végleges darabok felépítése
        const int mainLen   = bestMain.length_mm;
        const int toldasLen = need - mainLen;

        // FŐDARAB
        Cutting::Piece::PieceInfo mainInfo;
        mainInfo.length_mm  = mainLen;
        mainInfo.requestId  = req.requestId;
        mainInfo.toldasRole = ToldasRole::Main;

        if (!req.externalReference.isEmpty())
            mainInfo.externalReference = req.externalReference + " [REPLACE_THIN_MAIN]";
        else
            mainInfo.externalReference = "[REPLACE_THIN_MAIN]";

        if (bestMain.source == ThinMainCandidate::Source::Leftover) {
            mainInfo.keepWhole       = true;
            mainInfo.leftoverEntryId = bestMain.leftover.entryId;
        } else {
            mainInfo.keepWhole = false;
        }

        // TOLDAT
        Cutting::Piece::PieceInfo toldasInfo;
        toldasInfo.length_mm  = toldasLen;
        toldasInfo.requestId  = req.requestId;
        toldasInfo.toldasRole = ToldasRole::Toldat;

        if (!req.externalReference.isEmpty())
            toldasInfo.externalReference = req.externalReference + " [REPLACE_THIN_TOLDAT]";
        else
            toldasInfo.externalReference = "[REPLACE_THIN_TOLDAT]";

        if (!toldasFromStock)
            toldasInfo.leftoverEntryId = bestToldasLeftover.entryId;

        outPieces.append(Cutting::Piece::PieceWithMaterial(mainInfo, thinId));
        outPieces.append(Cutting::Piece::PieceWithMaterial(toldasInfo, thinId));

        return true;
    }



};

} // namespace Optimizer
} // namespace Cutting