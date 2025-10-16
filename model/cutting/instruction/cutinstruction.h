#pragma once

#include <QString>
#include <QUuid>
//#include <optional>

// Vágási utasítás státusz
enum class CutStatus {
    Pending,   ///< Még nem hajtották végre
    InProgress,///< Folyamatban (opcionális)
    Done,      ///< Végrehajtva
    Error      ///< Hiba történt
};

// Egyetlen vágái művelet végrehajtásának adatai
struct CutInstruction {
    QUuid rowId;                   // 🔹 UI-szintű azonosító (sorhoz kötve)

    int stepId = 0;                 // Folyamatos sorszám
    QString rodLabel;               // Rúd azonosító (A, B, C…)
    QUuid materialId;            // Anyag UUID
    //QString materialCode;           // Anyag kódja
    //QString materialName;           // Anyag neve
    QString barcode;                  // Konkrét rúd azonosítója
    double cutSize_mm = 0.0;        // Vágandó hossz
    double kerf_mm = 0.0;           // Vágásveszteség
    double remainingBefore_mm = 0.0;// Vágás előtti hossz
    double remainingAfter_mm = 0.0; // Vágás utáni hossz
    //QString machineName;            // Gépnélkülözhetetlen info
    QUuid machineId;            // Gép UUID
    CutStatus status = CutStatus::Pending; // 🔹 Enum alapú státusz

    // Segédfüggvény a számításhoz
    void computeRemaining() {
        remainingAfter_mm = remainingBefore_mm - cutSize_mm - kerf_mm;
    }

    CutInstruction() : rowId(QUuid::createUuid()) {}
};
