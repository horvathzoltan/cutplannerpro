#pragma once
#include <QString>
#include <QUuid>
#include <QSet>
#include <QVector>

struct CutPlanOutputSummary {

    int rodCount = 0;
    double rodTotal_m = 0.0;

    int pieceCount = 0;
    double pieceTotal_m = 0.0;

    int kerfCount = 0;
    int kerfTotal_mm = 0;

    int wasteCount = 0;
    int wasteTotal_mm = 0;

    int segmentCount = 0;

    int reusableCount = 0;
    int archivedCount = 0;

    double efficiency = 0.0;

    struct MaterialSummary {
        QUuid materialId;
        QString materialName;
        QString materialGroupName;
        int rodCount = 0;
        double rodTotal_m = 0.0;
        int pieceCount = 0;
        double pieceTotal_m = 0.0;
        QSet<QString> itemRefs;
    };

    QVector<MaterialSummary> materials;

    QString toText() const;
};
