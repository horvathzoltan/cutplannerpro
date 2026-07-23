#pragma once

#include "common/color/namedcolor.h"
#include <QHash>
#include <QUuid>


const double POFA_LENGTH_MM = 100.0;

struct PaintMaterialSummary {
    QUuid materialId;
    int totalPieces = 0;
    int totalLength_mm = 0;
    double powderKg = 0.0;
    QVector<QUuid> requestIds;

    bool hasCLT = false;   // jelzi, hogy tartozik lábtakaró is
};

struct PaintColorGroup {
    NamedColor color;
    QHash<QUuid, PaintMaterialSummary> materials; // key: materialId

    // --- TÍPUSONKÉNTI POFA ---
    int cipzarosPofa = 0;
    int sinesPofa = 0;
    int bowdenesPofa = 0;
    bool pofaFestheto = false;

    int sumPofa() const {return  cipzarosPofa + sinesPofa + bowdenesPofa;}
    int csavar = 0;
    bool csavarFestheto = false;

    double powderKg;
    double pofaPowderKg = 0.0;
};

struct PaintPlan {
    QHash<QString, PaintColorGroup> byColor; // key: color string
};

