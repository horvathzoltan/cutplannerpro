#ifndef CUTRESULT_H
#define CUTRESULT_H

#include "common/common.h"
#include <QString>
#include <QVector>

enum class LeftoverSource {
    Manual,
    Optimization
};

struct CutResult {
    int length = 0;           // Eredeti hossz (mm)
    QVector<int> cuts;        // Levágott darabok
    int waste = 0;            // Megmaradt anyag (mm)    - dokumentálja a vágás utáni megmaradt, levágatlan darabot
    ProfileCategory category;
    LeftoverSource source = LeftoverSource::Manual;
    std::optional<int> optimizationId; // csak ha source == Optimization

    QString cutsAsString() const;

    QString sourceAsString() const {
        if (source == LeftoverSource::Manual) {
            return QStringLiteral("Manuális");
        }
        if (source == LeftoverSource::Optimization) {
            return optimizationId.has_value()
            ? QStringLiteral("Op:%1").arg(*optimizationId)
            : QStringLiteral("Op:?");
        }
        return QStringLiteral("Ismeretlen");
    }

};

#endif // CUTRESULT_H
