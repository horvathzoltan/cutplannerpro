#pragma once

#include <QString>
#include <QUuid>

/**
 * @brief Egy relokációs dialógus egy sorát reprezentáló adatstruktúra.
 *
 * A RelocationQuantityRow egy tárhelyhez tartozó mennyiségkiosztási adatot ír le,
 * amelyet a felhasználó a dialógusban szerkeszthet. Lehet forrás (kivét) vagy cél (lerakás).
 *
 * Mezők:
 * - storageName: a tárhely megnevezése (pl. "Polc 14")
 * - available: elérhető mennyiség a tárhelyen (csak forrás esetén releváns)
 * - current: jelenlegi mennyiség a tárhelyen (csak cél esetén releváns)
 * - selected: a felhasználó által megadott mennyiség (kivét vagy lerakás)
 * - isTarget: jelzi, hogy a sor cél típusú-e (true = cél, false = forrás)
 */
struct RelocationQuantityRow {
    QUuid storageId;       ///< 🔹 a tárhely egyedi azonosítója
    QUuid entryId;         ///< forrás StockEntry azonosítója - a célban nincsen ilyen, a mozgatás során jön majd létre, vagy aggregál
    QString storageName;   ///< Tárhely neve (pl. "Polc 14")
    int available = 0;     ///< Elérhető készlet (csak forrás esetén releváns)
    int current = 0;       ///< Jelenlegi mennyiség (csak cél esetén releváns)
    int selected = 0;      ///< Felhasználó által megadott mennyiség (kivét vagy lerakás)
    bool isTarget = false; ///< true → cél tárhely, false → forrás tárhely
};
