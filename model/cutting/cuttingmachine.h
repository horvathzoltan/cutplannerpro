#pragma once

#include <QVector>
#include "../material/materialtype.h"
#include "../identifiableentity.h"

// 🔧 Gépdefiníció a vágáshoz: kerf, anyagkompatibilitás, hely, megjegyzés
struct CuttingMachine : public IdentifiableEntity {
    QString name;
    QString location;

    double kerf_mm = 0.0;                   // ✂️ Vágásveszteség mm-ben (kerf)
    double stellerMaxLength_mm = 0.0;       // 📏 Max hossz stellerrel
    double stellerCompensation_mm = 0.0;    // ⚖️ Kompenzáció érték mm-ben


    QVector<MaterialType> compatibleMaterials; // ⚙️ Alkalmas anyagtípusok (pl. Aluminium, Steel)

    QString comment;                        // 🗒️ Opcionális megjegyzés / karbantartási információ

    QUuid rootStorageId; // 🗂️ A géphez tartozó tárolófa gyökér StorageEntry ID-je

    CuttingMachine();
};

struct CuttingMachineMaterialRow {
    QString machineName;
    QString materialTypeStr;
};

/*
cuttingmachines.csv

name,location,kerf_mm,stellerMaxLength_mm,stellerCompensation_mm,comment
Körfűrész VAS,"Napellenzős műhely",3.2,0.0,0.0,"Kézi mérés, jelölés"
Körfűrész ALU,"Alu vágó",2.1,3200,2.4,"Stelleres vágás aluhoz és műanyaghoz"


cuttingmachine_materials.csv

machineName,materialTypeStr
Körfűrész VAS,Steel
Körfűrész ALU,Aluminium
Körfűrész ALU,Plastic
*/
