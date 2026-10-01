#pragma once

#include <QString>
#include <QUuid>
#include <QList>

#include "materials/model/material_paintgeometry_type.h"
#include "model/identifiableentity.h"

struct MaterialPaintGroup : public IdentifiableEntity {

private:
    QList<QUuid> materialIds;       // 📦 Hozzátartozó anyagok GUID-ja

public:
    std::optional<int> extraCut_mm = 0.0;  // +40 mm, ha nem függeszthető fel

    int dimA_mm = 0.0;        // Rectangle: szélesség, Diameter: átmérő
    std::optional<int> dimB_mm = 0.0;        // Rectangle: magasság, Linear/Diameter: 0

    PaintGeometryType geometry = PaintGeometryType::Linear;

    void addMaterial(const QUuid v){
        if(materialIds.contains(v)) return; // Elkerüljük a duplikációt
        materialIds.append(v);
    }

    bool contains(const QUuid& id) const {
        return materialIds.contains(id);
    }

    int size() const {
        return materialIds.size();
    }

    const QList<QUuid>& members() const{
        return materialIds;
    }

    QString toString_cm() const
    {
        const int a_cm = dimA_mm / 10;
        const int b_cm = dimB_mm.value_or(0) / 10;

        switch (geometry) {

        case PaintGeometryType::Linear:
            return QString("%1 cm").arg(a_cm);

        case PaintGeometryType::Rectangle:
            return QString("%1×%2 cm").arg(a_cm).arg(b_cm);

        case PaintGeometryType::Diameter:
            return QString("Ø%1 mm").arg(dimA_mm);

        case PaintGeometryType::Custom:
            return QString("%1 cm").arg(a_cm);
        }

        return QString("%1 mm").arg(dimA_mm);
    }
};
