#include "cuttingplanfinalizer.h"

/**
 * @brief Vágási tervek és hulladékok finalizálása — ez végzi a készletváltozást.
 *
 * Főbb lépések:
 *  1. Minden felhasznált darabot levon a stockból vagy reusable készletből
 *  2. Minden releváns hulladékot bejegyez a reusable stockba
 */
void CuttingPlanFinalizer::finalize(QVector<CutPlan>& plans,
                                    const QVector<CutResult>& leftovers)
{
    for (CutPlan& plan : plans) {
        if (plan.usedReusable()) {
            ReusableStockRegistry::instance().consume(plan.rodId);
        } else {
            StockRegistry::instance().consume(plan.materialId);
        }
        plan.setStatus(CutPlanStatus::Completed);
    }

    for (const CutResult& result : leftovers) {
        if (result.waste >= 300 && !result.reusableBarcode.isEmpty()) {
            ReusableStockEntry entry = CutResultUtils::toReusableEntry(result);
            ReusableStockRegistry::instance().add(entry);
        }
    }
}

