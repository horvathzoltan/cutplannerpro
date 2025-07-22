#include "cuttingplanfinalizer.h"

// 🔽 Készletregiszterek
#include "../model/registries/stockregistry.h"
#include "../model/registries/reusablestockregistry.h"

// 🔽 Konverziós logika: CutResult → ReusableStockEntry
#include "../common/cutresultutils.h"

// 🔽 Exportáló modul
#include "../common/archivedwasteutils.h"

void CuttingPlanFinalizer::finalize(QVector<CutPlan>& plans,
                                    const QVector<CutResult>& leftovers)
{
    // 1️⃣ Vágási tervek lezárása, alapanyagok fogyása
    for (CutPlan& plan : plans) {
        if (plan.usedReusable()) {
            ReusableStockRegistry::instance().consume(plan.rodId);
        } else {
            StockRegistry::instance().consume(plan.materialId);
        }
        plan.setStatus(CutPlanStatus::Completed);
    }

    // 2️⃣ Hulladékok feldolgozása
    QVector<ArchivedWasteEntry> archivedBatch;

    for (const CutResult& result : leftovers) {
        if (result.waste >= 300 && !result.reusableBarcode.isEmpty()) {
            // Használható darab → bekerül a reusable stockba
            ReusableStockEntry entry = CutResultUtils::toReusableEntry(result);
            ReusableStockRegistry::instance().add(entry);
        } else {
            // Túl rövid → archiválásra kerül
            ArchivedWasteEntry archived;
            archived.materialId        = result.materialId;
            archived.wasteLength_mm    = result.waste;
            archived.sourceDescription = result.sourceAsString(); // enum → string konverzió
            archived.createdAt         = QDateTime::currentDateTime();
            archived.group             = QString();               // nincs group → üres
            archived.originBarcode     = result.reusableBarcode;
            archived.note              = "Finalize: túl rövid hulló → archiválva";
            archived.cutPlanId         = result.cutPlanId;                // nincs cutPlanId → üres UUID

            archivedBatch.append(archived);
        }
    }

    // 3️⃣ Archiválás fájlba, ha van selejt
    if (!archivedBatch.isEmpty()) {
        ArchivedWasteUtils::exportToCSV(archivedBatch);
    }
}
