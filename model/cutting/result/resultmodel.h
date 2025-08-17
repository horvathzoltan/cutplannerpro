#pragma once

//#include "common/common.h"
#include "../piece/piecewithmaterial.h"
#include "model/cutting/result/resultsource.h"
#include "model/material/materialtype.h"
#include <QColor>
#include <QString>
#include <QUuid>
#include <QVector>

namespace Cutting{
namespace Result{

struct ResultModel {
    QUuid materialId;               // 🔗 Törzsből visszakereshető anyag
    int length = 0;                 // 📏 Eredeti rúd hossza
    QVector<Cutting::Piece::PieceWithMaterial> cuts;             // ✂️ Levágott darabok
    int waste = 0;                 // ♻️ Maradék (levágatlan anyag)
    //LeftoverSource source = LeftoverSource::Undefined;
    ResultSource source;
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

}}
