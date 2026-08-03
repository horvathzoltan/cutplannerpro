#pragma once

#include <QUuid>
#include <QStringList>
#include <QDate>

#include "../../../common/color/namedcolor.h"
#include "common/surface/surfacetype.h"
#include "relevantdimension.h"
#include "tolerance.h"

namespace Cutting {
namespace Plan {

struct Request {
    QUuid requestId = QUuid::createUuid(); // 💡 Automatikus egyedi azonosító
    QUuid materialId;           ///< 🔗 Az anyag egyedi törzsbeli azonosítója
    int requiredLength;         ///< 📏 Vágás hossza (milliméterben) nominális méret
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
    RelevantDimension relevantDim = RelevantDimension::Width;
    //QString requiredColorName;    /// ha ez eltér a material colorjától, szinterezni kell ->
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

    std::optional<Tolerance> requiredTolerance;

    int leftCount = 0;   ///< Balos darabok száma
    int rightCount = 0;  ///< Jobbos darabok száma

    //Subtype subtype = Subtype::None; ///< Szerkezeti elem típusa (Alap, Rugós, Tetőteríti, stb.)

    QUuid productTypeId;        ///< 🔗 A termék típusa (ProductType)
    QUuid productSubtypeId;     ///< 🔗 A termék altípusa (ProductSubtype)

    // 🎨 Anyag színe - ebben a színben kéri a megrendelő a terméket (RAL vagy HEX kód)
    NamedColor requiredColor;
    SurfaceType surface;
    //QString color;

    QDate dueDate = QDate::currentDate();   // 🗓️ alapértelmezés: ma


    QMap<QString, QString> attributes;

    void setAttribute(const QString& key, const QString& value) {
        attributes[key] = value;
    }

    QString getAttribute(const QString& key) const {
        return attributes.value(key);
    }

    QString getAttributes(){

    }
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

    QString displayText();

    QString attributesToString() const
    {
        if (attributes.isEmpty())
            return "{}";

        QStringList parts;

        // 🔒 determinisztikus sorrend (audit miatt kötelező)
        QStringList keys = attributes.keys();
        std::sort(keys.begin(), keys.end(), [](const QString& a, const QString& b){
            return a.localeAwareCompare(b) < 0;
        });

        for (const QString& key : keys) {
            parts << QString("%1=%2").arg(key, attributes.value(key));
        }

        return parts.join("; ");
    }

    QString toString2() const;
};
} //endof namespace Plan
} //endof namespace Cutting
