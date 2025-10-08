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
static constexpr int Material    = 0;   ///< Anyag neve
static constexpr int Barcode     = 1;   ///< Vonalkód
static constexpr int Quantity    = 2;   ///< Mennyiség / státusz
static constexpr int Source      = 3;   ///< Forrás tároló
static constexpr int Target      = 4;   ///< Cél tároló
static constexpr int Type        = 5;   ///< Forrás típusa (Stock/Hulló)
static constexpr int Finalize    = 6;   ///< Forrás típusa (Stock/Hulló)
}
