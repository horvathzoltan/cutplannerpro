#pragma once

//#include "model/storageaudit/storageauditrow.h"
#include <QString>
#include <QUuid>
//#include <optional>
#include <QVector>

/**
 * @brief Egy relokációs művelet forrás tárhelyét reprezentáló adatstruktúra.
 *
 * A RelocationSourceEntry tartalmazza, hogy egy adott tárhelyen mennyi készlet áll rendelkezésre,
 * és abból mennyit mozgattunk el a relokáció során.
 *
 * Mezők:
 * - locationId: a forrás tárhely egyedi azonosítója (UUID)
 * - locationName: a tárhely megnevezése (pl. "Polc 14")
 * - available: a tárhelyen elérhető készlet mennyisége
 * - moved: a felhasználó által megadott, ténylegesen elmozgatott mennyiség
 */
struct RelocationSourceEntry {
    QUuid entryId;          ///< Konkrét StockEntry azonosító
    QUuid locationId;       ///< Forrás tárhely azonosító
    QString locationName;   ///< Forrás tárhely neve
    int available = 0;      ///< Elérhető mennyiség a stockban
    int moved = 0;          ///< Innen ténylegesen elmozgatott mennyiség
};
