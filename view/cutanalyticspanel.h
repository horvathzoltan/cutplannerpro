#pragma once

#include <QFrame>
#include <QLabel>
#include <QVector>
#include "../model/cutting/plan/cutplan.h"
#include "../model/cutting/result/resultmodel.h"

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
    void updateStats(const QVector<Cutting::Plan::CutPlan>& plans, const QVector<Cutting::Result::ResultModel>& leftovers);

private:
    QLabel* lblSummary;
    QLabel* lblSegments;
    QLabel* lblFinalWaste;
    QLabel* lblEfficiency;

    void setupLayout();
};
