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

    view->updateStats(model.getPlans(), model.getLeftoverResults());
}

QVector<CutPlan> CuttingPresenter::getPlans()
{
    return model.getPlans();
}

QVector<CutResult> CuttingPresenter::getLeftoverResults()
{
    return model.getLeftoverResults();
}

namespace CuttingUtils {
void logStockStatus(const QString& title, const QVector<StockEntry>& entries) {
    qDebug() << title;
    for (const StockEntry& e : entries)
        qDebug() << "  MaterialId:" << e.materialId << "| Quantity:" << e.quantity;
}

void logReusableStatus(const QString& title, const QVector<ReusableStockEntry>& entries) {
    qDebug() << title;
    for (const ReusableStockEntry& e : entries)
        qDebug() << "  Barcode:" << e.barcode << "| Length:" << e.availableLength_mm << "| Group:" << e.groupName();
}
}

/*finalize*/

void CuttingPresenter::finalizePlans()
{
    //const QVector<CutPlan> plans = model.getPlans();
    QVector<CutPlan>& plans = model.getPlansRef(); // vagy getMutablePlans()
    const QVector<CutResult> results = model.getLeftoverResults();

    qDebug() << "✅ VÁGÁSI TERVEK — CutPlan-ek:";
    for (const CutPlan& plan : plans) {
        QStringList pieceLabels, kerfLabels, wasteLabels;

        for (const Segment& s : plan.segments) {
            switch (s.type) {
            case SegmentType::Piece:  pieceLabels << s.toLabelString(); break;
            case SegmentType::Kerf:   kerfLabels  << s.toLabelString(); break;
            case SegmentType::Waste:  wasteLabels << s.toLabelString(); break;
            }
        }

        qDebug().nospace()
            << "  → #" << plan.rodNumber
            << " | PlanId: " << plan.planId
            << " | Forrás: " << (plan.source == CutPlanSource::Reusable ? "♻️ REUSABLE" : "🧱 STOCK")
            << "\n     Azonosító: " << (plan.usedReusable() ? plan.rodId : plan.name())
            << " | Vágások száma: " << plan.cuts.size()
            << " | Kerf: " << plan.kerfTotal << " mm"
            << " | Hulladék: " << plan.waste << " mm"
            << "\n     Darabok: " << pieceLabels.join(" ")
            << "\n     Kerf-ek: " << kerfLabels.join(" ")
            << "\n     Hulladék szakaszok: " << wasteLabels.join(" ");
    }

    qDebug() << "♻️ KELETKEZETT HULLADÉKOK — CutResult-ek:";
    for (const CutResult& result : results) {
        qDebug().nospace()
        << "  - Hulladék: " << result.waste << " mm"
        << " | Forrás: " << result.sourceAsString()
        << " | MaterialId: " << result.materialId
        << " | Barcode: " << result.reusableBarcode
        << "\n    Darabok: " << result.cutsAsString();
    }

    // 📊 Összesítés
    int totalKerf = 0, totalWaste = 0, totalCuts = 0;
    int totalSegments = 0, kerfSegs = 0, wasteSegs = 0;

    for (const CutPlan& plan : plans) {
        totalKerf += plan.kerfTotal;
        totalWaste += plan.waste;
        totalCuts += plan.cuts.size();
        totalSegments += plan.segments.size();

        for (const Segment& s : plan.segments) {
            if (s.type == SegmentType::Kerf)  kerfSegs++;
            if (s.type == SegmentType::Waste) wasteSegs++;
        }
    }

    qDebug().nospace() << "📈 Összesítés:\n"
                       << "  Vágások összesen:         " << totalCuts << "\n"
                       << "  Kerf összesen:            " << totalKerf << " mm (" << kerfSegs << " szakasz)\n"
                       << "  Hulladék összesen:        " << totalWaste << " mm (" << wasteSegs << " szakasz)\n"
                       << "  Teljes szakaszszám:       " << totalSegments;

    qDebug() << "***";

    CuttingUtils::logStockStatus("🧱 STOCK — finalize előtt:", StockRegistry::instance().all());
    CuttingUtils::logReusableStatus("♻️ REUSABLE — finalize előtt:", ReusableStockRegistry::instance().all());

    // ✂️ Finalizálás → készletfogyás + hulladékkezelés
    CuttingPlanFinalizer::finalize(plans, results);

    qDebug() << "***";

    CuttingUtils::logStockStatus("🧱 STOCK — finalize után:", StockRegistry::instance().all());
    CuttingUtils::logReusableStatus("♻️ REUSABLE — finalize után:", ReusableStockRegistry::instance().all());

    // ✅ Állapot lezárása
    for (CutPlan& plan : model.getPlansRef())
        plan.setStatus(CutPlanStatus::Completed);

    // 🔁 View frissítése
    if (view) {
        view->update_stockTable();
        view->update_leftoversTable(CutResultUtils::toReusableEntries(results));
        view->update_ResultsTable(plans);
    }
}

// CuttingPresenter.cpp




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








