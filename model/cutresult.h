#ifndef CUTRESULT_H
#define CUTRESULT_H

#include "common/common.h"
#include "model/materialtype.h"
#include <QColor>
#include <QString>
#include <QUuid>
#include <QVector>

enum class LeftoverSource {
    Manual,
    Optimization
};

struct CutResult {
    QUuid materialId;               // 🔗 Törzsből visszakereshető anyag
    int length = 0;                 // 📏 Eredeti rúd hossza
    QVector<int> cuts;             // ✂️ Levágott darabok
    int waste = 0;                 // ♻️ Maradék (levágatlan anyag)
    LeftoverSource source = LeftoverSource::Manual;
    std::optional<int> optimizationId;  // Csak ha source == Optimization

    QString cutsAsString() const;
    QString sourceAsString() const;

    MaterialType materialType() const;      // 🔎 törzsből lekérhető típus
    QString materialName() const;           // 🧪 megjelenítéshez
    QColor categoryColor() const;           // 🎨 badge háttér (UI-hoz)
};

#endif // CUTRESULT_H
