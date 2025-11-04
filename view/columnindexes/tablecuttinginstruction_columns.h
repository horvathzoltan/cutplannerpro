#pragma once

/**
 * @brief Oszlopindexek a CuttingInstruction (Worklist) táblához.
 *
 * Ezeket a konstansokat használjuk a QTableWidget oszlopainak
 * eléréséhez, hogy ne kelljen "magic number"-öket szórni a kódba.
 *
 * Oszlopok:
 * - StepId: lépés azonosító (operátor ezzel kattintja készre)
 * - RodId: rúd azonosító (emberi olvasásra, pl. Rod-A1)
 * - Material: anyag + csoport + barcode (MaterialCellGenerator formátumban)
 * - LengthBefore: vágás előtti hossz
 * - CutSize: vágandó hossz
 * - LengthAfter: vágás utáni hossz
 * - Status: Pending / InProgress / Done
 * - Finalize: végrehajtás gomb vagy ikon
 */
namespace CuttingInstructionTableColumns {

enum Column {
    StepId = 0,          ///< Lépés azonosító
    RodId,               ///< Rúd azonosító (pl. Rod-A1)
    Material,            ///< Anyag + csoport + barcode (egységes cella)
    LengthBefore,     ///< Vágás előtti hossz (mm)
    CutSize,             ///< Vágandó hossz (mm)
    LengthAfter,      ///< Vágás utáni hossz (mm)
    Status,              ///< Pending / InProgress / Done
    Finalize             ///< Végrehajtás gomb vagy ikon
};

}
