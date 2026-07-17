#pragma once
#include <QVector>
#include <QUuid>
#include "materials/model/material_master.h"
#include "model/cutting/plan/request.h"

class MaterialSelector
{
public:
    // preferált anyag kiválasztása
    static QUuid selectPreferred(
        const QVector<QUuid>& bomList,
        const Cutting::Plan::Request& req);

    // teljes rangsor visszaadása (opcionális)
    static QVector<QUuid> rankMaterials(
        const QVector<QUuid>& bomList,
        const Cutting::Plan::Request& req);
};
