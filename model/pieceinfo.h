#pragma once

#include <QString>

/**
 * @brief Egy darabolási munkadarab részletes információi
 */
struct PieceInfo
{
    int length_mm = 0;                // 📏 Hossz milliméterben
    QString ownerName;                // 👤 Megrendelő, tulajdonos
    QString externalReference;        // 📎 Külső tételszám, SAP vagy egyedi azonosító
    bool isCompleted = false;         // ✅ Elkészült-e a darab

    bool isValid() const {
        return length_mm > 0 && !ownerName.isEmpty();
    }

    QString displayText() const {
        return QString("%1 • %2 • %3 mm")
            .arg(ownerName.isEmpty() ? "Ismeretlen" : ownerName)
            .arg(externalReference.isEmpty() ? "-" : externalReference)
            .arg(length_mm);
    }
};
