#include "CuttingPresenter.h"
#include "../view/MainWindow.h"
#include "common/cutresultutils.h"
#include "common/optimizationexporter.h"
#include "model/archivedwasteentry.h"
#include "common/archivedwasteutils.h"

#include <model/registries/cuttingplanrequestregistry.h>
#include <model/registries/leftoverstockregistry.h>
#include <model/registries/stockregistry.h>

#include <common/cuttingplanfinalizer.h>
#include <common/filenamehelper.h>
#include <common/settingsmanager.h>



CuttingPresenter::CuttingPresenter(MainWindow* view, QObject *parent)
    : QObject(parent), view(view) {}

void CuttingPresenter::createNewCuttingPlan() {
    QString newFileName = FileNameHelper::instance().getNew_CuttingPlanFileName();
    QString newFilePath = FileNameHelper::instance().getCuttingPlanFilePath(newFileName);

    // 🔄 Állapot frissítése
    SettingsManager::instance().setCuttingPlanFileName(newFileName);

    clearCuttingPlan();

    // 🧹 GUI frissítés
    if (view) {
        view->setInputFileLabel(newFileName, newFilePath);
    }
}

void CuttingPresenter::clearCuttingPlan() {
    // 🧹 Táblázat törlése a GUI-ban
    if (view) {
        view->clear_InputTable();
    }
    // 🗃️ Registry kiürítése
    CuttingPlanRequestRegistry::instance().clear();
}

/*input*/
void CuttingPresenter::addCutRequest(const CuttingPlanRequest& req) {
    CuttingPlanRequestRegistry::instance().registerRequest(req);
    if(view){
         view->addRow_InputTable(req);
    }
 }

void CuttingPresenter::updateCutRequest(const CuttingPlanRequest& r) {
    bool ok = CuttingPlanRequestRegistry::instance().updateRequest(r); // 🔁 adatbázis update

    if (ok){
        if(view){
            view->updateRow_InputTable(r);
        }
    }
    else
    {
         qWarning() << "❌ Sikertelen frissítés: nincs ilyen requestId:" << r.requestId;
         return;
     }

 }

void CuttingPresenter::removeCutRequest(const QUuid& requestId) {
    CuttingPlanRequestRegistry::instance().removeRequest(requestId);  // ✅ Globális törlés
    if(view){
        view->removeRow_InputTable(requestId);
    }
}

/*stock*/
void CuttingPresenter::removeStockEntry(const QUuid& stockId) {
    StockRegistry::instance().remove(stockId);   // ✅ Globális törlés
    if (view) {
        view->removeRow_StockTable(stockId); // ha a készlet változik
    }}

void CuttingPresenter::updateStockEntry(const StockEntry& updated) {
    bool ok = StockRegistry::instance().update(updated); // 🔁 adatbázis update

    if (ok){
        if(view){
            view->updateRow_StockTable(updated);
        }
    }
    else
    {
        qWarning() << "❌ Sikertelen frissítés: nincs ilyen entryId:" << updated.entryId;
        return;
    }
}

/*waste*/
void CuttingPresenter::removeLeftoverEntry(const QUuid& entryId) {
    bool ok = LeftoverStockRegistry::instance().removeByEntryId(entryId);

    if (ok){
        if(view){
            view->removeRow_LeftoversTable(entryId);
        }
    }
    else
    {
        qWarning() << "❌ Sikertelen törlés: nincs ilyen entryId:" << entryId;
        return;
    }
}

void CuttingPresenter::updateLeftoverEntry(const LeftoverStockEntry& updated) {
    bool ok = LeftoverStockRegistry::instance().update(updated); // 🔁 Frissítés Registry-ben

    if (ok) {
        if (view) {
            view->updateRow_LeftoversTable(updated);
        }
    }
    else
    {
        qWarning() << "❌ Sikertelen frissítés: nincs ilyen entryId:" << updated.entryId;
        return;
    }
}


void CuttingPresenter::setCuttingRequests(const QVector<CuttingPlanRequest>& list) {
    model.setCuttingRequests(list);
}

void CuttingPresenter::setStockInventory(const QVector<StockEntry>& list) {
    model.setStockInventory(list);
}

void CuttingPresenter::setReusableInventory(const QVector<LeftoverStockEntry>& list) {
    model.setReusableInventory(list);
}

void CuttingPresenter::setKerf(int kerf) {
    model.setKerf(kerf);
}

QVector<CutPlan>& CuttingPresenter::getPlansRef()
{
    return model.getResult_PlansRef();
}

QVector<CutResult> CuttingPresenter::getLeftoverResults()
{
    return model.getResults_Leftovers();
}

// void CuttingPresenter::clearRequests() {
//     model.clearRequests();
// }

void CuttingPresenter::runOptimization() {
    if (!isModelSynced) {
        qWarning() << "⚠️ A modell nem volt szinkronizálva optimalizáció előtt!";
        // opcionálisan: return vagy default szinkron
        return;
    }

    model.optimize();
    isModelSynced = false; // újra false az állapot, ha később újra hívnák

    QVector<CutPlan> &plans = model.getResult_PlansRef();

    // ✨ Ha készen állsz rá, itt frissíthetjük a View táblákat:
    if (view) {
        // ez a közéspső - eredmény tábla
        view->update_ResultsTable(plans);
        // ez a készlet
        view->update_StockTable(); // ha a készlet változik
        // ez a maradék

        QVector<CutResult> l = model.getResults_Leftovers();
        QVector<LeftoverStockEntry> e = CutResultUtils::toReusableEntries(l);

        // todo 01 nem jó, a stockot kellene frissíteni - illetve opt után kell-e bármit is, hisz majd a finalize frissít - nem?
        view->update_LeftoversTable();//e);
    }
    OptimizationExporter::exportPlansToCSV(plans);
    OptimizationExporter::exportPlansAsWorkSheetTXT(plans);

    view->updateStats(plans, model.getResults_Leftovers());
}

namespace CuttingUtils {
void logStockStatus(const QString& title, const QVector<StockEntry>& entries) {
    qDebug() << title;
    for (const StockEntry& e : entries)
        qDebug() << "  MaterialId:" << e.materialId << "| Quantity:" << e.quantity;
}

void logReusableStatus(const QString& title, const QVector<LeftoverStockEntry>& entries) {
    qDebug() << title;
    for (const LeftoverStockEntry& e : entries)
        qDebug() << "  Barcode:" << e.barcode << "| Length:" << e.availableLength_mm << "| Group:" << e.materialGroupName();
}
}

/*finalize*/

void CuttingPresenter::finalizePlans()
{
    //const QVector<CutPlan> plans = model.getPlans();
    QVector<CutPlan>& plans = model.getResult_PlansRef(); // vagy getMutablePlans()
    const QVector<CutResult> results = model.getResults_Leftovers();

    qDebug() << "✅ VÁGÁSI TERVEK — CutPlan-ek:";
    for (const CutPlan& plan : plans) {
        QStringList pieceLabels, kerfLabels, wasteLabels;

        for (const Segment& s : plan.segments) {
            switch (s.type) {
            case Segment::Type::Piece:  pieceLabels << s.toLabelString(); break;
            case Segment::Type::Kerf:   kerfLabels  << s.toLabelString(); break;
            case Segment::Type::Waste:  wasteLabels << s.toLabelString(); break;
            }
        }

        qDebug().nospace()
            << "  → #" << plan.rodNumber
            << " | PlanId: " << plan.planId
            << " | Forrás: " << (plan.source == CutPlanSource::Reusable ? "♻️ REUSABLE" : "🧱 STOCK")
            << "\n     Azonosító: " << (plan.usedReusable() ? plan.rodId : plan.materialName())
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
            if (s.type == Segment::Type::Kerf)  kerfSegs++;
            if (s.type == Segment::Type::Waste) wasteSegs++;
        }
    }

    qDebug().nospace() << "📈 Összesítés:\n"
                       << "  Vágások összesen:         " << totalCuts << "\n"
                       << "  Kerf összesen:            " << totalKerf << " mm (" << kerfSegs << " szakasz)\n"
                       << "  Hulladék összesen:        " << totalWaste << " mm (" << wasteSegs << " szakasz)\n"
                       << "  Teljes szakaszszám:       " << totalSegments;

    qDebug() << "***";

    CuttingUtils::logStockStatus("🧱 STOCK — finalize előtt:", StockRegistry::instance().all());
    CuttingUtils::logReusableStatus("♻️ REUSABLE — finalize előtt:", LeftoverStockRegistry::instance().all());

    // ✂️ Finalizálás → készletfogyás + hulladékkezelés
    CuttingPlanFinalizer::finalize(plans, results);

    qDebug() << "***";

    CuttingUtils::logStockStatus("🧱 STOCK — finalize után:", StockRegistry::instance().all());
    CuttingUtils::logReusableStatus("♻️ REUSABLE — finalize után:", LeftoverStockRegistry::instance().all());

    // ✅ Állapot lezárása
    for (CutPlan& plan : model.getResult_PlansRef())
        plan.setStatus(CutPlanStatus::Completed);

    // 🔁 View frissítése
    if (view) {
        view->update_StockTable();
        // todo 02 : nem jó, nem a táblát kellene frissíteni, hanem a stockot
        view->update_LeftoversTable();//CutResultUtils::toReusableEntries(results));
        view->update_ResultsTable(plans);
    }
}

void CuttingPresenter::scrapShortLeftovers()
{
    auto& reusableRegistry = LeftoverStockRegistry::instance();
    QVector<ArchivedWasteEntry> archivedEntries;
    QVector<LeftoverStockEntry> toBeScrapped;

    for (const LeftoverStockEntry &entry : reusableRegistry.all()) {
        if (entry.availableLength_mm < 300) {
            ArchivedWasteEntry archived;
            archived.materialId = entry.materialId;
            archived.wasteLength_mm = entry.availableLength_mm;
            archived.sourceDescription = "Selejtezés reusable készletből";
            archived.createdAt = QDateTime::currentDateTime();
            archived.group = entry.materialGroupName();
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

void CuttingPresenter::syncModelWithRegistries() {
    auto requestList  = CuttingPlanRequestRegistry::instance().readAll();
    auto stockList    = StockRegistry::instance().all();
    auto reusableList = LeftoverStockRegistry::instance().filtered(300);

    QStringList errors;

    // 📋 Validációs hibák aggregálása
    if (requestList.isEmpty())
        errors << "Nincs megadva vágási igény.";

    if (stockList.isEmpty())
        errors << "A készlet üres.";

    if (reusableList.isEmpty())
        errors << "Nincs újrahasználható hulladék elérhető.";

    // ❗ Hibaüzenetek megjelenítése
    if (errors.isEmpty()){
        // 🔁 Modellbe betöltés
        model.setCuttingRequests(requestList);
        model.setStockInventory(stockList);
        model.setReusableInventory(reusableList);

        isModelSynced = true;
    } else {
        QString fullMessage = "Az optimalizálás nem indítható:\n\n• " + errors.join("\n• ");
        if(view)
            view->ShowWarningDialog(fullMessage);
        isModelSynced = false;
    }
}








