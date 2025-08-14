#pragma once

#include <QUuid>
#include <QString>


/**
 * @brief Egy vágási igényt reprezentáló adatstruktúra.
 *
 * Tartalmazza az anyag azonosítóját, a kívánt hosszúságot, darabszámot,
 * valamint opcionálisan a megrendelő nevét és a külső hivatkozási azonosítót.
 */

namespace Cutting {
namespace Plan {


struct Request {
    QUuid requestId = QUuid::createUuid(); // 💡 Automatikus egyedi azonosító
    QUuid materialId;           ///< 🔗 Az anyag egyedi törzsbeli azonosítója
    int requiredLength;         ///< 📏 Vágás hossza (milliméterben)
    int quantity;               ///< 🔢 Szükséges darabszám

    QString ownerName;          ///< 👤 Megrendelő neve (opcionális információ)
    QString externalReference;  ///< 🧾 Külső hivatkozás / tételszám (opcionális információ)

    /**
     * @brief Ellenőrzi, hogy az igény érvényes-e.
     *
     * @return true ha minden kötelező mező értelmes adatot tartalmaz.
     */
    bool isValid() const;

    /**
     * @brief Visszaadja az érvénytelenség okát szöveges formában.
     *
     * @return QString Magyarázat az invalid állapotra.
     */
    QStringList invalidReasons() const;


    QString toString() const;
};
} //endof namespace Plan
} //endof namespace Cutting
