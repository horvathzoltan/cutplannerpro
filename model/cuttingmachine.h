#pragma once

#include <QVector>
#include "materialtype.h"
#include "identifiableentity.h"

// 🔧 Gépdefiníció a vágáshoz: kerf, anyagkompatibilitás, hely, megjegyzés
struct CuttingMachine : public IdentifiableEntity {
    double kerf_mm = 0.0;                   // ✂️ Vágásveszteség mm-ben (kerf)
    QVector<MaterialType> compatibleMaterials; // ⚙️ Alkalmas anyagtípusok (pl. Aluminium, Steel)
    QString location;                       // 📍 Gép telephelye vagy zóna (pl. "Műhely 2")
    QString comment;                        // 🗒️ Opcionális megjegyzés / karbantartási információ

    CuttingMachine();
};
