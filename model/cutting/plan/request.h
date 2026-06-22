#pragma once

#include <QUuid>
#include <QStringList>
#include <QDate>

#include "../../../common/color/namedcolor.h"


/**
 * @brief Egy vágási igényt reprezentáló adatstruktúra.
 *
 * Tartalmazza az anyag azonosítóját, a kívánt hosszúságot, darabszámot,
 * valamint opcionálisan a megrendelő nevét és a külső hivatkozási azonosítót.
 */
enum HandlerSide{
    Left,    ///< Balos kivitel – kezelő/hajtómű bal oldalon
    Right,   ///< Jobbos kivitel – kezelő/hajtómű jobb oldalon
    None   ///< Nem megadott – figyelmeztetés szükséges
};

namespace HandlerSideUtils {

inline QString toDisplayText(HandlerSide side){
    if(side == HandlerSide::Left) return "bal";
    if(side == HandlerSide::Right) return "jobb";
    return {};
}

// inline QString toCSVText(HandlerSide side){
//     if(side == HandlerSide::Left) return "L";
//     if(side == HandlerSide::Right) return "R";
//     return {};
// }

inline HandlerSide parse(const QString& str2) {
    QString str = str2.trimmed().toLower();
    if (str == "left" || str == "l")   return HandlerSide::Left;
    if (str == "right" || str == "r")  return HandlerSide::Right;

    if (str == "bal" || str =="b")   return HandlerSide::Left;
    if (str == "jobb" || str=="j")  return HandlerSide::Right;
    return HandlerSide::None;
}

} // namespace HandlerSideUtils

enum class Subtype {
    None,
    Alap,
    Rugos,
    Tetoteri
};

namespace SubtypeUtils {

inline QString toString_CSV(Subtype t) {
    switch (t) {
    case Subtype::None:     return "none";
    case Subtype::Alap:     return "alap";
    case Subtype::Rugos:    return "rugos";
    case Subtype::Tetoteri: return "tetoteri";
    }
    return "none";
}

inline QString toDisplayText(Subtype t) {
    switch (t) {
    case Subtype::None:     return "none";
    case Subtype::Alap:     return "alap";
    case Subtype::Rugos:    return "rugós";
    case Subtype::Tetoteri: return "tetőtéri";
    }
    return "none";
}

inline Subtype parse(const QString& s) {
    QString v = s.trimmed().toLower();
    if (v == "alap")     return Subtype::Alap;
    if (v == "rugos")    return Subtype::Rugos;
    if (v == "tetoteri") return Subtype::Tetoteri;
    return Subtype::None;
}

} //endof namespace SubtypeUtils


struct Tolerance{
    double min_mm; ///< negatív eltérés mm-ben
    double max_mm; ///< pozitív eltérés mm-ben

    // CSV-hez: csak a tűrés szintaxis
    QString toCsvString(bool withUnit = false) const {
        QString unit = withUnit ? " mm" : "";
        if (qFuzzyCompare(std::abs(min_mm), std::abs(max_mm)) && min_mm < 0 && max_mm > 0) {
            return QString("+/-%1%2").arg(max_mm).arg(unit);
        }
        return QString("%1/%2%3").arg(min_mm).arg(max_mm).arg(unit);
    }

    // Emberi olvasásra: nominális mérettel kombinálva
    QString toString(double nominal = 0, bool withUnit = true) const {
        QString unit = withUnit ? " mm" : "";
        if (qFuzzyCompare(std::abs(min_mm), std::abs(max_mm)) && min_mm < 0 && max_mm > 0) {
            if (nominal != 0) {
                return QString("%1 ±%2%3").arg(nominal).arg(max_mm).arg(unit);
            }
            return QString("+/-%1%2").arg(max_mm).arg(unit);
        }
        if (nominal != 0) {
            return QString("%1 %2/%3%4").arg(nominal).arg(min_mm).arg(max_mm).arg(unit);
        }
        return QString("%1/%2%3").arg(min_mm).arg(max_mm).arg(unit);
    }

    static std::optional<Tolerance> fromString(const QString& s) {
        if (s.isEmpty()) return std::nullopt;
        QString str = s.trimmed();

        if (str.startsWith("+/-")) {
            bool ok = false;
            double val = str.mid(3).toDouble(&ok);
            if (ok) return Tolerance{ -val, val };
            return std::nullopt;
        }

        if (str.contains("±")) {
            auto parts = str.split("±");
            if (parts.size() == 2) {
                bool ok = false;
                double val = parts[1].remove("mm").trimmed().toDouble(&ok);
                if (ok) return Tolerance{ -val, val };
            }
        }

        auto parts = str.split('/');
        if (parts.size() == 2) {
            bool ok1 = false, ok2 = false;
            double minVal = parts[0].remove("mm").trimmed().toDouble(&ok1);
            double maxVal = parts[1].remove("mm").trimmed().toDouble(&ok2);
            if (ok1 && ok2) return Tolerance{ minVal, maxVal };
        }

        return std::nullopt;
    }
};

enum class RelevantDimension { Width, Height };

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

    //HandlerSide handlerSide = HandlerSide::Unknown; ///< kezelő/hajtómű oldala (bal/jobb/ismeretlen)
    int leftCount = 0;   ///< Balos darabok száma
    int rightCount = 0;  ///< Jobbos darabok száma

    Subtype subtype = Subtype::None; ///< Szerkezeti elem típusa (Alap, Rugós, Tetőteríti, stb.)

    // 🎨 Anyag színe - ebben a színben kéri a megrendelő a terméket (RAL vagy HEX kód)
    NamedColor requiredColor;

    QDate dueDate = QDate::currentDate();   // 🗓️ alapértelmezés: ma

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
};
} //endof namespace Plan
} //endof namespace Cutting
