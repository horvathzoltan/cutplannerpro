#pragma once

#include "model/storageaudit/storageauditrow.h"
#include <QString>
#include <QUuid>
#include <optional>
#include <QVector>

/**
 * @brief Egy relokációs művelet cél tárhelyét reprezentáló adatstruktúra.
 *
 * A RelocationTargetEntry tartalmazza, hogy egy adott cél tárhelyre mennyi anyagot helyeztünk el
 * a relokáció során. Akkor is szerepelhet, ha jelenleg nincs rajta készlet.
 *
 * Mezők:
 * - locationId: a cél tárhely egyedi azonosítója (UUID)
 * - locationName: a tárhely megnevezése (pl. "Vasanyag vágó")
 * - placed: a felhasználó által megadott, ténylegesen lerakott mennyiség
 */
struct RelocationTargetEntry {
    QUuid locationId;       ///< Cél tárhely azonosító
    QString locationName;   ///< Cél tárhely neve
    int placed = 0;         ///< Ide ténylegesen lerakott mennyiség
};
