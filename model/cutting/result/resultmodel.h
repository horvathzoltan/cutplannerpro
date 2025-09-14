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

// 1. ez azért kell, mert
// 2. ResultModel = egy darab, a vágás során frissen keletkező hulladékot jelent
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

    //QUuid leftoverEntryId; // csak ha source == FromReusable

    QString cutsAsString() const;
    QString sourceAsString() const;

    MaterialType materialType() const;      // 🔎 törzsből lekérhető típus
    QString materialName() const;           // 🧪 megjelenítéshez
    QColor materialGroupColor() const;           // 🎨 badge háttér (UI-hoz)
};

}}
