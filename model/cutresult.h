#ifndef CUTRESULT_H
#define CUTRESULT_H

//#include "common/common.h"
#include "model/cutting/piecewithmaterial.h"
#include "model/materialtype.h"
#include <QColor>
#include <QString>
#include <QUuid>
#include <QVector>

enum class LeftoverSource {
    Manual,
    Optimization,
    Undefined
};

enum class CutResultSource {
    FromStock,      // 🧱 Szálanyagból jött hulladék
    FromReusable,   // ♻️ Használt reusable darabból jött
    Unknown          // ❓ Ha nem egyértelmű
};

struct CutResult {
    QUuid materialId;               // 🔗 Törzsből visszakereshető anyag
    int length = 0;                 // 📏 Eredeti rúd hossza
    QVector<PieceWithMaterial> cuts;             // ✂️ Levágott darabok
    int waste = 0;                 // ♻️ Maradék (levágatlan anyag)
    //LeftoverSource source = LeftoverSource::Undefined;
    CutResultSource source;
    std::optional<int> optimizationId;  // Csak ha source == Optimization

    QString reusableBarcode;

    QUuid cutPlanId; // 🔗 Az eredeti vágási terv azonosítója
    bool isFinalWaste = false; // ✅ új mező

    QString cutsAsString() const;
    QString sourceAsString() const;

    MaterialType materialType() const;      // 🔎 törzsből lekérhető típus
    QString materialName() const;           // 🧪 megjelenítéshez
    QColor materialGroupColor() const;           // 🎨 badge háttér (UI-hoz)
};

#endif // CUTRESULT_H
