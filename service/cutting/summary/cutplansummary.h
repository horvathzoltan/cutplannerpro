#pragma once

#include <QSet>
#include <QString>
#include <QUuid>
#include <QVector>


struct CutPlanSummary {

    int rodCount = 0;          // hány rúd lett felhasználva
    double rodTotal_m = 0.0;   // rudak teljes hossza méterben

    int pieceCount = 0;        // levágott darabok száma (Piece szegmensek)
    double pieceTotal_m = 0.0; // levágott darabok teljes hossza méterben

    int kerfCount = 0;         // kerf szegmensek száma (vágások száma)
    int kerfTotal_mm = 0;      // kerf teljes hossza mm-ben

    int wasteCount = 0;        // hulladék szegmensek száma
    int wasteTotal_mm = 0;     // hulladék teljes hossza mm-ben

    int segmentCount = 0;      // összes szegmens (Piece + Kerf + Waste)

    int reusableCount = 0;     // újrahasználható leftoverek száma (>= 300 mm)
    int archivedCount = 0;     // végmaradék leftoverek száma (isFinalWaste)

    double efficiency = 0.0;   // anyagfelhasználási hatékonyság (%)


    struct MaterialSummary {
        QUuid materialId;
        QString materialName;
        QString materialGroupName;
        int rodCount = 0;
        double rodTotal_m = 0.0;
        int pieceCount = 0;
        double pieceTotal_m = 0.0;

        QSet<QString> itemRefs;   // ← tételszámok (externalReference)
    };

    QVector<MaterialSummary> materials;   // anyagonkénti összefoglaló

    QString toText() const;
};
