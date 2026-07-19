#pragma once
#include <QVector>
#include <QUuid>
//#include "materials/model/material_master.h"
#include "model/cutting/plan/request.h"

class MaterialSelector
{
public:
    struct ScoreBreakdown {
        int colorExact = 0;
        int colorNat = 0;
        int colorLightness = 0;
        int colorPenalty = 0;

        int axisPref = 0;
        int stockPref = 0;

        int total() const {
            return colorExact + colorNat + colorLightness + colorPenalty +
                   axisPref + stockPref;
        }
    };

    // preferált anyag kiválasztása
    static QUuid selectPreferred(
        const QVector<QUuid>& bomList,
        const Cutting::Plan::Request& req);

    // teljes rangsor visszaadása (opcionális)
    static QVector<QUuid> rankMaterials(
        const QVector<QUuid>& bomList,
        const Cutting::Plan::Request& req);
};
