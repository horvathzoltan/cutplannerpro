#pragma once

#include "model/cutting/plan/request.h"
#include "model/cutting/plan/source.h"
#include <QString>
#include <QUuid>
#include <QVector>

// Vágási utasítás státusz
enum class CutStatus {
    Pending,   ///< Még nem hajtották végre
    InProgress,///< Ez az aktuális sor, itt van a zöld gomb
    Done,      ///< Végrehajtva
    Error      ///< Hiba történt
};

// Egyetlen vágái művelet végrehajtásának adatai
struct CutInstruction {
    QUuid rowId;                   // 🔹 UI-szintű azonosító (sorhoz kötve)

    int globalStepId = 0;                 // Folyamatos sorszám
    QString rodId;               // Rúd azonosító (A, B, C…)
    QUuid materialId;            // Anyag UUID

    QString barcode;                // Konkrét rúd azonosítója
    double cutSize_mm = 0.0;        // Vágandó hossz
    double kerf_mm = 0.0;           // Vágásveszteség
    double lengthBefore_mm = 0.0;// Vágás előtti hossz
    double lengthAfter_mm = 0.0; // Vágás utáni hossz

//    QUuid machineId;        // Gép UUID
    //QString machineName;     // Gép neve (UI-hoz, redundáns viewmodel mező)
    CutStatus status = CutStatus::Pending;

    bool isFinalLeftover = false; // 🔴 Végső leftover jelző
    QString leftoverBarcode;
    bool isManualCut = false;
    double effectiveCutSize_mm = 0.0; // kompenzációval számolt méret

    QUuid requestId;        // 🔗 Request azonosító - az eredeti, ahol kérünk egy tételszámon ötöt
    QString externalReference;     // ⭐ darab-szintű externalReference (pl. 1444.1/5)

    HandlerSide side = HandlerSide::None; ///< kezelő/hajtómű oldala (bal/jobb/ismeretlen)
    //Subtype subtype = Subtype::None; ///< Szerkezeti elem típusa (Alap, Rugós, Tetőteríti, stb.)

    QUuid productTypeId;
    QUuid productSubtypeId;
    QMap<QString, QString> attributes; ///< termékhez tartozó további attribútumok (pl. szín, felület, stb.)

    // 🔹 Új: tényleges darab azonosító
    int pieceCounter = 0;
    // 🔹 Új: finalize-kor mentett kompenzáció
    double appliedCompensation_mm;

    // Segédfüggvény a számításhoz
    void computeRemaining() {
        lengthAfter_mm = lengthBefore_mm - cutSize_mm - kerf_mm;
    }

    Cutting::Plan::Source source = Cutting::Plan::Source::Stock;         // 🔥 új
    QString sourceBarcode;                        // 🔥 új
    //std::optional<QUuid> leftoverEntryId;         // 🔥 új

    CutInstruction() : rowId(QUuid::createUuid()) {}
};

struct MachineHeader {
    QUuid machineId;
    QString machineName;
    QString comment;
    double kerf_mm = 0.0;
    std::optional<double> stellerMaxLength_mm;
    std::optional<double> stellerCompensation_mm;
};


struct MachineCuts{
    MachineHeader machineHeader;
    QVector<CutInstruction> cutInstructions;


};
