#pragma once

/**
 * @brief Column indexek a RelocationPlan táblához.
 *
 * Ezeket a konstansokat használjuk a QTableWidget oszlopainak
 * eléréséhez, hogy ne kelljen "magic number"-öket szórni a kódba.
 *
 * Oszlopok:
 * - Material: anyag neve
 * - Barcode: vonalkód (stock aggregátum vagy hulló egyedi azonosító)
 * - Quantity: mennyiség vagy státusz (✔ Megvan)
 * - Source: forrás tároló
 * - Target: cél tároló
 * - Type: forrás típusa (Stock / Hulló)
 */
namespace RelocationPlanTableColumns {
enum Columns{
    Material = 0,   ///< Anyag neve + csoport + barcode
    Quantity,   ///< Mennyiség / státusz
    Source,   ///< Forrás tároló
    Target,   ///< Cél tároló
    Type,   ///< Forrás típusa (Stock/Hulló)
    Finalize,   ///< Forrás típusa (Stock/Hulló)
};
} // endof namespace RelocationPlanTableColumns
