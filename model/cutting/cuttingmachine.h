#pragma once

#include <QVector>
#include "../material/materialtype.h"
#include "../identifiableentity.h"

// 🔧 Gépdefiníció a vágáshoz: kerf, anyagkompatibilitás, hely, megjegyzés
struct CuttingMachine : public IdentifiableEntity {
    CuttingMachine() = default;

    QString name;
    QString location;

    double kerf_mm = 0.0;                   // ✂️ Vágásveszteség mm-ben (kerf)
    std::optional<double> stellerMaxLength_mm = std::nullopt;       // 📏 Max hossz stellerrel
    std::optional<double> stellerCompensation_mm = std::nullopt;    // ⚖️ Kompenzáció érték mm-ben

    QVector<MaterialType> compatibleMaterials; // ⚙️ Alkalmas anyagtípusok (pl. Aluminium, Steel)

    QUuid rootStorageId;    // 🗂️ A géphez tartozó tárolófa gyökér StorageEntry ID-je    
    QString comment;        // 🗒️ Opcionális megjegyzés / karbantartási információ
    
    void addMaterialType(const MaterialType& v);
};

