#pragma once

#include <QTableWidget>
#include <model/cutresult.h>
#include "../model/materialmaster.h"
#include "../model/cuttingrequest.h"
//#include "grouputils.h"
#include "model/cutplan.h"
#include "model/reusablestockentry.h"

// 🎨 Táblázatsor stíluskezelő – közös utility
class RowStyler {
public:
    static void applyInputStyle(QTableWidget* table, int row, const MaterialMaster* mat, const CuttingRequest& request);
    static void applyStockStyle(QTableWidget* table, int row, const MaterialMaster* mat) ;
//    static void applyLeftoverStyle(QTableWidget* table, int row, const MaterialMaster* master, const CutResult& res);
    static void applyReusableStyle(QTableWidget *table, int row, const MaterialMaster *master, const ReusableStockEntry &entry);
    static void applyResultStyle(QTableWidget *table, int row, const MaterialMaster *mat, const CutPlan &plan);
    static void applyBadgeBackground(QWidget *widget, const QColor &base);
};
