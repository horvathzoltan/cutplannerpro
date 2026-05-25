#pragma once

#include "../piece/piecewithmaterial.h"
#include "resultsource.h"
#include "materials/model/material_type.h"
#include <QColor>
#include <QString>
#include <QUuid>
#include <QVector>
#include <model/cutting/plan/parentinfo.h>

namespace Cutting{
namespace Result{

/**
 * @class ResultModel: egy vágási művelet eredménye
 * @brief A vágás újonnan keletkezett!! maradékanyagának - hullójának - leírója
 * - tartalmazza a felhasznált anyagot, a levágott darabokat, és a maradékot
 * - újrahasznosítás és UI megjelenítés céljából használjuk
 * - ez alapján auditálni anyagfelhasználást nem lehet,
 * de be lehet vezetni mint hullót a leftoverstockba pl.
 * ami azután felhasználható optimalizáláskor.
 *
 *  OptimizerModel::_result_leftovers -be kerülnek az eredmények
*/
struct ResultModel {
    QUuid materialId;               // 🔗 Törzsből visszakereshető anyag
    int length = 0;                 // 📏 Eredeti rúd hossza
    QVector<Cutting::Piece::PieceWithMaterial> cuts;  // ✂️ Levágott darabok
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

    std::optional<Cutting::Plan::ParentInfo> _parent = std::nullopt;

    QString sourceBarcode; // miből vágtuk ténylegesen
    bool isReusable() const { return source == ResultSource::FromReusable; }
};

}}
