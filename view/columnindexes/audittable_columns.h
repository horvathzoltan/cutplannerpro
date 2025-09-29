#pragma once

/**
 * @brief Column indexek a StorageAudit táblához.
 *
 * Ezek a konstansok határozzák meg a QTableWidget oszlopainak sorrendjét
 * és jelentését az audit nézetben. Így elkerülhető a "magic number"
 * használata, és a kód olvashatóbb, karbantarthatóbb lesz.
 *
 * Oszlopok:
 * - Material: anyag neve + szín (csoport színkód alapján)
 * - Barcode: vonalkód (csak megjelenítés, nem interaktív)
 * - Storage: tároló neve (ahonnan vagy ahová az anyag kerül)
 * - Expected: elvárt mennyiség (összesített vagy egyedi)
 * - Actual: tényleges mennyiség (interaktív: SpinBox vagy rádiógomb)
 * - Missing: hiányzó mennyiség (Expected - Actual)
 * - Status: audit státusz (színezett cella: OK, Missing, Pending)
 */
namespace AuditTableColumns {
static constexpr int Material = 0;  ///< Anyag neve + szín (csoport színkód alapján)
static constexpr int Barcode  = 1;  ///< Vonalkód (csak megjelenítés, nem interaktív)
static constexpr int Storage  = 2;  ///< Tároló neve (ahonnan vagy ahová az anyag kerül)
static constexpr int Expected = 3;  ///< Elvárt mennyiség (összesített vagy egyedi)
static constexpr int Actual   = 4;  ///< Tényleges mennyiség (interaktív: SpinBox vagy rádiógomb)
static constexpr int Missing  = 5;  ///< Hiányzó mennyiség (Expected - Actual)
static constexpr int Status   = 6;  ///< Audit státusz (színezett cella: OK, Missing, Pending)
}
