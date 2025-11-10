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
enum Column{
    Material = 0,  ///< Anyag neve + szín (csoport színkód alapján) + barcode
    Storage,  ///< Tároló neve (ahonnan vagy ahová az anyag kerül)
    Expected,  ///< Elvárt mennyiség (összesített vagy egyedi)
    Actual,  ///< Tényleges mennyiség (interaktív: SpinBox vagy rádiógomb)
    Missing,  ///< Hiányzó mennyiség (Expected - Actual)
    Status,  ///< Audit státusz (színezett cella: OK, Missing, Pending)
};
}//endof namespace AuditTableColumns
