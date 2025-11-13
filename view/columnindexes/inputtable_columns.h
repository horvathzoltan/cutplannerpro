#pragma once

/**
 * @brief Column indexek a CutRequest (InputTable) táblához.
 *
 * Ezek a konstansok határozzák meg a QTableWidget oszlopainak sorrendjét
 * és jelentését a vágási igények nézetében. Így elkerülhető a "magic number"
 * használata, és a kód olvashatóbb, karbantarthatóbb lesz.
 *
 * Oszlopok:
 * - Owner: megrendelő neve
 * - ExternalRef: külső hivatkozás / tételszám
 * - Length: nominális vágási hossz (mm)
 * - Tolerance: tűrés szöveges formátumban (pl. +/-0.5)
 * - Quantity: darabszám
 * - Color: igényelt szín (ha eltér a material színétől)
 * - Measurement: mérési terv jelző (kötelező mérés ✔ vagy üres)
 * - Actions: műveletek (Update/Delete gombok)
 */
namespace InputTableColumns {
enum Column {
    Owner = 0,       ///< Megrendelő neve
    ExternalRef,     ///< Külső hivatkozás / tételszám
    Material,
    Length,          ///< Nominális vágási hossz (mm)
    Tolerance,       ///< Tűrés szöveges formátumban
    Quantity,        ///< Darabszám
    HandlerSide, /// < Kezelő oldal (Left/Right)
    Color,           ///< Igényelt szín
    Measurement,     ///< Mérési terv jelző (checkbox vagy ikon)
    Actions          ///< Műveletek (Update/Delete gombok)
};
} // end of namespace InputTableColumns
