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
    QString ownerName;                // 👤 Megrendelő, tulajdonos
    QString externalReference;        // 📎 Külső tételszám, SAP vagy egyedi azonosító
    bool isCompleted = false;         // ✅ Elkészült-e a darab

    bool isValid() const {
        return length_mm > 0 && !ownerName.isEmpty();
    }

    QString displayText() const {
        return QString("%1 • %2 • %3 mm")
            .arg(ownerName.isEmpty() ? "(?)" : ownerName)
            .arg(externalReference.isEmpty() ? "-" : externalReference)
            .arg(length_mm);
    }
};

} // endof namespace Piece
} // endof namespace Cutting
