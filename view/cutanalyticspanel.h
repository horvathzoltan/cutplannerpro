#pragma once

#include <QFrame>
#include <QLabel>
#include <QVector>
#include "../model/cutplan.h"
#include "../model/cutresult.h"

/**
 * @brief Vágási statisztikák megjelenítő panel
 */
class CutAnalyticsPanel : public QFrame
{
    Q_OBJECT

public:
    explicit CutAnalyticsPanel(QWidget* parent = nullptr);

    /**
     * @brief Frissíti a panel tartalmát a vágási tervek és hulladékok alapján
     */
    void updateStats(const QVector<CutPlan>& plans, const QVector<CutResult>& leftovers);

private:
    QLabel* lblSummary;
    QLabel* lblSegments;
    QLabel* lblFinalWaste;
    QLabel* lblEfficiency;

    void setupLayout();
};
