#pragma once

#include "model/cutting/optimizer/toldasrole.h"
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

    std::optional<QUuid> leftoverEntryId;
    ToldasRole toldasRole;
    bool keepWhole = false;
};

} // endof namespace Piece
} // endof namespace Cutting
