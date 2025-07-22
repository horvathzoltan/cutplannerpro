#include "CuttingPresenter.h"
#include "../view/MainWindow.h"
#include "common/cutresultutils.h"
#include "common/optimizationexporter.h"
#include "model/archivedwasteentry.h"
#include "common/archivedwasteutils.h"

#include <model/registries/reusablestockregistry.h>
#include <model/registries/stockregistry.h>

#include <common/cuttingplanfinalizer.h>


CuttingPresenter::CuttingPresenter(MainWindow* view, QObject *parent)
    : QObject(parent), view(view) {}

void CuttingPresenter::addCutRequest(const CuttingRequest& req) {
    model.addRequest(req);
}

void CuttingPresenter::setCuttingRequests(const QVector<CuttingRequest>& list) {
    model.setRequests(list);
}

void CuttingPresenter::setStockInventory(const QVector<StockEntry>& list) {
    model.setStockInventory(list);
}

void CuttingPresenter::setReusableInventory(const QVector<ReusableStockEntry>& list) {
    model.setReusableInventory(list);
}

void CuttingPresenter::setKerf(int kerf) {
    model.setKerf(kerf);
}

void CuttingPresenter::clearRequests() {
    model.clearRequests();
}

void CuttingPresenter::runOptimization() {
    model.optimize();

    // ✨ Ha készen állsz rá, itt frissíthetjük a View táblákat:
    if (view) {
        // ez a közéspső - eredmény tábla
        view->update_ResultsTable(model.getPlans());
        // ez a készlet
        view->update_stockTable(); // ha a készlet változik
        // ez a maradék

        QVector<CutResult> l = model.getLeftoverResults();
        QVector<ReusableStockEntry> e = CutResultUtils::toReusableEntries(l);
        view->update_leftoversTable(e);

    }
    OptimizationExporter::exportPlansToCSV(model.getPlans());
    OptimizationExporter::exportPlansAsWorkSheetTXT(model.getPlans());
}

QVector<CutPlan> CuttingPresenter::getPlans()
{
    return model.getPlans();
}

QVector<CutResult> CuttingPresenter::getLeftoverResults()
{
    return model.getLeftoverResults();
}

/*finalize*/

void CuttingPresenter::finalizePlans()
{
    QVector<CutPlan> plans     = model.getPlans();
    QVector<CutResult> results = model.getLeftoverResults();

    qDebug() << "✅ VÁGÁSI TERVEK — CutPlan-ek:";
    for (const CutPlan& plan : plans) {
        qDebug() << "  → #" << plan.rodNumber
                 << "| PlanId:" << plan.planId.toString()
                 << "| Azonosító:" << (plan.usedReusable() ? plan.rodId : plan.name())
                 << "| Darabok:" << plan.cuts
                 << "| Kerf összesen:" << plan.kerfTotal << "mm"
                 << "| Hulladék:" << plan.waste << "mm"
                 << "| Forrás:" << (plan.source == CutPlanSource::Reusable ? "REUSABLE" : "STOCK")
                 << "| Státusz:" << static_cast<int>(plan.getStatus())
                 << "| Barcode:" << plan.rodId;
            ;
    }

    qDebug() << "♻️ KELETKEZETT HULLADÉKOK — CutResult-ek:";
    for (const CutResult& result : results) {
        qDebug() << "  - Hulladék:" << result.waste << "mm"
                 << "| Forrás:" << result.sourceAsString()
                 << "| MaterialId:" << result.materialId
                 << "| Barcode:" << result.reusableBarcode;
    }

    int totalKerf = 0;
    int totalWaste = 0;
    int totalCuts = 0;

    for (const CutPlan& plan : plans) {
        totalKerf += plan.kerfTotal;
        totalWaste += plan.waste;
        totalCuts += plan.cuts.size();
    }

    qDebug() << "📈 Összesítés: "
             << "Darabolások:" << totalCuts
             << "| Kerf összesen:" << totalKerf << "mm"
             << "| Hulladék összesen:" << totalWaste << "mm";

    qDebug() << "***";


    qDebug() << "🧱 STOCK — finalize előtt:";
    for (const StockEntry& entry : StockRegistry::instance().all()) {
        qDebug() << "  MaterialId:" << entry.materialId << " | Quantity:" << entry.quantity;
    }

    qDebug() << "♻️ REUSABLE — finalize előtt:";
    for (const ReusableStockEntry& entry : ReusableStockRegistry::instance().all()) {
        qDebug() << "  Barcode:" << entry.barcode
                 << " | Length:" << entry.availableLength_mm
                 << " | Group:" << entry.groupName();
    }


    CuttingPlanFinalizer::finalize(plans, results);

    qDebug() << "***";

    qDebug() << "🧱 STOCK — finalize után:";
    for (const StockEntry& entry : StockRegistry::instance().all()) {
        qDebug() << "  MaterialId:" << entry.materialId << " | Quantity:" << entry.quantity;
    }

    qDebug() << "♻️ REUSABLE — finalize után:";
    for (const ReusableStockEntry& entry : ReusableStockRegistry::instance().all()) {
        qDebug() << "  Barcode:" << entry.barcode
                 << " | Length:" << entry.availableLength_mm
                 << " | Group:" << entry.groupName();
    }


    for (CutPlan& plan : plans) {
        plan.setStatus(CutPlanStatus::Completed);
    }

    // 🎯 Ha szeretnéd: visszafrissítés View felé is
    if (view) {
        view->update_stockTable();
        view->update_leftoversTable(CutResultUtils::toReusableEntries(results));
        view->update_ResultsTable(plans);
    }
}

void CuttingPresenter::scrapShortLeftovers()
{
    auto& reusableRegistry = ReusableStockRegistry::instance();
    QVector<ArchivedWasteEntry> archivedEntries;
    QVector<ReusableStockEntry> toBeScrapped;

    for (const ReusableStockEntry &entry : reusableRegistry.all()) {
        if (entry.availableLength_mm < 300) {
            ArchivedWasteEntry archived;
            archived.materialId = entry.materialId;
            archived.wasteLength_mm = entry.availableLength_mm;
            archived.sourceDescription = "Selejtezés reusable készletből";
            archived.createdAt = QDateTime::currentDateTime();
            archived.group = entry.groupName();
            archived.originBarcode = entry.barcode;
            archived.note = "Nem használható → archiválva";
            archived.cutPlanId = QUuid(); // ha nincs konkrét terv

            archivedEntries.append(archived);
            toBeScrapped.append(entry);
        }
    }

    for (const auto& e : toBeScrapped)
        reusableRegistry.consume(e.barcode);

    if (!archivedEntries.isEmpty())
        ArchivedWasteUtils::exportToCSV(archivedEntries);
}








