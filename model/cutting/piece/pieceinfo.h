#pragma once

#include <QString>
#include <QUuid>

/**
 * @brief Egy darabolási munkadarab részletes információi
 */

namespace Cutting {
namespace Piece {

// Egy darab leíró model
// darabon itt a vágandó darabot értjük
// a Cutting::Plan::Request -ben lévő adatokat használjuk mint vágandó darabot

struct PieceInfo
{
    QUuid pieceId = QUuid::createUuid(); // ✅ automatikus UUID generálás;
    int length_mm = 0;                // 📏 Hossz milliméterben
    QUuid requestId;               // 🔗 Eredeti igény azonosító
    bool isCompleted = false;         // ✅ Elkészült-e a darab

    QString externalReference;   // ⭐ darab-szintű tételszám (pl. 1444.1/5)

    // PATCH — ToldasEngine fődarab jelölése
    // Ha true, akkor a CutEngine nem vágja meg a darabot.
    bool keepWhole = false;

    // PATCH 11/A — leftover eredet jelölése
    // Ha a darab leftoverből jött, itt tároljuk a leftover entryId-ját.
    std::optional<QUuid> leftoverEntryId;
    // PATCH 12 — Toldas szerepkör jelölése
    // Lehetséges értékek:
    //   ""              → nem toldás
    //   "TOLDAS_MAIN"   → fődarab
    //   "TOLDAS_TOLDAT" → toldat
    QString toldasRole;

};

} // endof namespace Piece
} // endof namespace Cutting
