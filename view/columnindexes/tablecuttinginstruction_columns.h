#pragma once

/**
 * @brief Column indexek a CuttingInstruction (Worklist) táblához.
 *
 * Ezeket a konstansokat használjuk a QTableWidget oszlopainak
 * eléréséhez, hogy ne kelljen "magic number"-öket szórni a kódba.
 *
 * Oszlopok:
 * - StepId: lépés azonosító
 * - RodLabel: rúd jelölése (A, B, C…)
 * - Material: anyag neve
 * - Barcode: konkrét rúd azonosító
 * - CutSize: vágandó hossz
 * - RemainingBefore: vágás előtti hossz
 * - RemainingAfter: vágás utáni hossz
 * - Machine: gép neve
 * - Status: Pending / Done
 * - Finalize: végrehajtás gomb vagy státusz
 */
namespace CuttingInstructionTableColumns {
static constexpr int StepId          = 0;  ///< Lépés azonosító
static constexpr int RodLabel        = 1;  ///< Rúd jelölése
static constexpr int Material        = 2;  ///< Anyag neve
static constexpr int Barcode         = 3;  ///< Vonalkód
static constexpr int CutSize         = 4;  ///< Vágandó hossz (mm)
static constexpr int RemainingBefore = 5;  ///< Vágás előtti hossz (mm)
static constexpr int RemainingAfter  = 6;  ///< Vágás utáni hossz (mm)
static constexpr int Machine         = 7;  ///< Gép neve
static constexpr int Status          = 8;  ///< Pending / Done
static constexpr int Finalize        = 9;  ///< Végrehajtás gomb
}
