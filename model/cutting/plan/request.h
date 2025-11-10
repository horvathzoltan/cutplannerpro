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

    // a minitokos sávrolónál vagy rolettánál a minitokhoz
    // tartozik két függőleges takarólemez, "láb" is
    // ezeknél a függöleges méret a releváns,
    // míg egyéb szerkezeti elemnél a vízszintes - hiszen karnis elemek,
    // és a karnis maga vízszintes, vízszintesen kerül felszerelésre
    int fullWidth_mm = 0;    ///< Teljes szélesség mm-ben (opcionális)
    int fullHeight_mm = 0;   ///< Teljes magasság mm-ben (opcionális)
    enum class RelevantDimension { Width, Height };
    RelevantDimension relevantDim = RelevantDimension::Width;
    QString requiredColorName;    /// ha ez eltér a material colorjától, szinterezni kell ->
// ha szinterezni kell, akkor plusz költség van,
// ami költség arányos a festett felülettel
// emiatt plusz számítás van
// plusz időbe telik
// az elemeket fel kell fűzni vagy függeszteni a festéshez
// azon elemeknél, ahol ez nem lehetséges,
// 2-5-10 cm-el nagyobbra kell vágni, furatozni vagy csavarozni kell a függeszték miatt
// és ha visszajön a festésből, kell gondoskodni a méretre vágásról és a sorjázásról
// illetve a termék utána szerelhető össze
    bool isMeasurementNeeded = false; ///< ha igaz, akkor vágás után mérni kell a pontos méretet
    // azaz ezt az elemet biztosan bele kell vennünk a
    // vágási utasítást követő és/vagy ahoz tartozó , abból származó mérési tervbe

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

    int primaryDimension() const {
        return (relevantDim == RelevantDimension::Width) ? fullWidth_mm : fullHeight_mm;
    }

    QString toString() const;
};
} //endof namespace Plan
} //endof namespace Cutting
