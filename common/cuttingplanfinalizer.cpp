#include "cuttingplanfinalizer.h"

// 🔽 Készletregiszterek
#include "../model/registries/stockregistry.h"
#include "../model/registries/leftoverstockregistry.h"

// 🔽 Konverziós logika: CutResult → ReusableStockEntry
#include "../common/cutresultutils.h"

// 🔽 Exportáló modul
#include "../common/archivedwasteutils.h"
#include "common/segmentutils.h"

void CuttingPlanFinalizer::finalize(QVector<CutPlan>& plans,
                                    const QVector<CutResult>& leftovers)
{
    // 1️⃣ A vágási tervek lezárása, és az alapanyag „fogyasztása” készletből
    for (CutPlan& plan : plans) {
        if (plan.usedReusable()) {
            // ♻️ Ha hullóból vágtunk → annak eltávolítása
            LeftoverStockRegistry::instance().consumeEntry(plan.rodId);
        } else {
            // 🧱 Ha eredeti profilból vágtunk → készlet csökkentése
            StockRegistry::instance().consumeEntry(plan.materialId);
        }

        plan.setStatus(CutPlanStatus::Completed); // ✅ Állapot frissítése: kész
    }

    // 2️⃣ Hulladékok feldolgozása → újrahasználat vagy archiválás
    QVector<ArchivedWasteEntry> archivedBatch;

    for (const CutResult& result : leftovers) {
        if (result.waste >= 300 && !result.reusableBarcode.isEmpty()) {
            // ✅ Elég hosszú → bekerül az újrahasználható rúdlistába
            LeftoverStockEntry reusable = CutResultUtils::toReusableEntry(result);
            LeftoverStockRegistry::instance().registerEntry(reusable);
        } else {
            // 🗂️ Rövid → archiválandó hulladékként tároljuk

            // 🔍 Eredeti CutPlan előkeresése planId alapján
            auto it = std::find_if(plans.begin(), plans.end(), [&](const CutPlan& p) {
                return p.planId == result.cutPlanId;
            });

            // 📜 Megállapítjuk, hogy ez valóban végmaradék volt-e
            bool trailingWaste = false;
            if (it != plans.end()) {
                trailingWaste = SegmentUtils::isTrailingWaste(result.waste, it->segments);
            }

            // 📝 Archivált selejt felépítése
            ArchivedWasteEntry archived;
            archived.materialId        = result.materialId;
            archived.wasteLength_mm    = result.waste;
            archived.sourceDescription = result.sourceAsString(); // pl. „FromStock”
            archived.createdAt         = QDateTime::currentDateTime();
            archived.group             = QString();               // Nincs csoportozás
            archived.originBarcode     = result.reusableBarcode;
            archived.cutPlanId         = result.cutPlanId;

            archived.isFinalWaste = trailingWaste;

            // 🗒️ Pontos megjegyzés generálása
            archived.note = trailingWaste
                                ? "Finalize: végmaradék a záró vágásból"
                                : "Finalize: túl rövid hulló → archiválva";

            archivedBatch.append(archived);
        }
    }

    // 3️⃣ Exportálás CSV fájlba, ha történt archiválás
    if (!archivedBatch.isEmpty()) {
        ArchivedWasteUtils::exportToCSV(archivedBatch);
    }
}

