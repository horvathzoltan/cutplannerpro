#pragma once

#include <QString>


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
        return "";
    }

    inline bool tryParse(const QString& str2, HandlerSide& out)
    {
        const QString str = str2.trimmed().toLower();

        if (str == "left" || str == "l" ||
            str == "bal"  || str == "b")
        {
            out = HandlerSide::Left;
            return true;
        }

        if (str == "right" || str == "r" ||
            str == "jobb"  || str == "j")
        {
            out = HandlerSide::Right;
            return true;
        }

        return false;
    }

} // namespace HandlerSideUtils