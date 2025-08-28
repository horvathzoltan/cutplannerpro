#include "CuttingPresenter.h"
#include "../view/MainWindow.h"

#include "common/logger.h"
#include "model/registries/materialregistry.h"
#include "model/relocation/relocationinstruction.h"
#include "model/storageaudit/storageauditentry.h"
#include "service/cutting/result/resultutils.h"
#include "model/archivedwasteentry.h"
#include "model/registries/cuttingplanrequestregistry.h"
#include "model/registries/leftoverstockregistry.h"
#include "model/registries/stockregistry.h"

#include "service/cutting/plan/finalizer.h"
#include "service/cutting/optimizer/exporter.h"
#include "service/cutting/result/archivedwasteutils.h"

#include "common/filenamehelper.h"
#include "common/settingsmanager.h"

#include <model/repositories/cuttingrequestrepository.h>

#include <service/storageaudit/storageauditservice.h>

CuttingPresenter::CuttingPresenter(MainWindow* view, QObject *parent)
    : QObject(parent), view(view) {}

// ez új cutting plant csinál és új néven kezdi perzisztálni
void CuttingPresenter::createNew_CuttingPlanRequests() {
    QString newFileName = FileNameHelper::instance().getNew_CuttingPlanFileName();
    QString newFilePath = FileNameHelper::instance().getCuttingPlanFilePath(newFileName);

    // 🔄 Állapot frissítése
    SettingsManager::instance().setCuttingPlanFileName(newFileName);

    removeAll_CuttingPlanRequests();

    // 🧹 GUI frissítés - beírjuk az új file nevet a labelbe
    if (view) {
        view->setInputFileLabel(newFileName, newFilePath);
    }
}

void CuttingPresenter::removeAll_CuttingPlanRequests() {
    // 🧹 Táblázat törlése a GUI-ban
    if (view) {
        view->clear_InputTable();
    }
    // 🗃️ Registry kiürítése
    CuttingPlanRequestRegistry::instance().clearAll();
}

/*input*/
void CuttingPresenter::add_CuttingPlanRequest(const Cutting::Plan::Request& req) {
    CuttingPlanRequestRegistry::instance().registerRequest(req);
    if(view){
         view->addRow_InputTable(req);
    }
 }

void CuttingPresenter::update_CuttingPlanRequest(const Cutting::Plan::Request& r) {
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

void CuttingPresenter::remove_CuttingPlanRequest(const QUuid& requestId) {
    CuttingPlanRequestRegistry::instance().removeRequest(requestId);  // ✅ Globális törlés
    if(view){
        view->removeRow_InputTable(requestId);
    }
}

/*stock*/
void CuttingPresenter::add_StockEntry(const StockEntry& e) {
    StockRegistry::instance().registerEntry(e);
    if(view){
        view->addRow_StockTable(e);
    }
}

void CuttingPresenter::remove_StockEntry(const QUuid& stockId) {
    StockRegistry::instance().removeEntry(stockId);   // ✅ Globális törlés
    if (view) {
        view->removeRow_StockTable(stockId); // ha a készlet változik
    }
}

void CuttingPresenter::update_StockEntry(const StockEntry& updated) {
    bool ok = StockRegistry::instance().updateEntry(updated); // 🔁 adatbázis update

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

void CuttingPresenter::add_LeftoverStockEntry(const LeftoverStockEntry& req) {
    LeftoverStockRegistry::instance().registerEntry(req);
    if(view){
        view->addRow_LeftoversTable(req);
    }
}

void CuttingPresenter::remove_LeftoverStockEntry(const QUuid& entryId) {
    bool ok = LeftoverStockRegistry::instance().removeEntry(entryId);

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

void CuttingPresenter::update_LeftoverStockEntry(const LeftoverStockEntry& updated) {
    bool ok = LeftoverStockRegistry::instance().updateEntry(updated); // 🔁 Frissítés Registry-ben

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


void CuttingPresenter::setCuttingRequests(const QVector<Cutting::Plan::Request>& list) {
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

QVector<Cutting::Plan::CutPlan>& CuttingPresenter::getPlansRef()
{
    return model.getResult_PlansRef();
}

QVector<Cutting::Result::ResultModel> CuttingPresenter::getLeftoverResults()
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

    QVector<Cutting::Plan::CutPlan> &plans = model.getResult_PlansRef();

    // ✨ Ha készen állsz rá, itt frissíthetjük a View táblákat:
    if (view) {
        // ez a közéspső - eredmény tábla
        view->update_ResultsTable(plans);
        // ez a készlet
        view->refresh_StockTable(); // ha a készlet változik
        // ez a maradék

        QVector<Cutting::Result::ResultModel> l = model.getResults_Leftovers();
        QVector<LeftoverStockEntry> e = Cutting::Result::ResultUtils::toReusableEntries(l);

        // todo 01 nem jó, a stockot kellene frissíteni - illetve opt után kell-e bármit is, hisz majd a finalize frissít - nem?
        view->refresh_LeftoversTable();//e);
    }
    OptimizationExporter::exportPlansToCSV(plans);
    OptimizationExporter::exportPlansAsWorkSheetTXT(plans);

    view->updateStats(plans, model.getResults_Leftovers());

    auto pickingMap = generatePickingMapFromPlans(plans);
    for (auto it = pickingMap.begin(); it != pickingMap.end(); ++it) {
        QString msg = L("📦 Picking: %1 -> %2").arg(it.key()) .arg(it.value());
        zInfo(msg);
    }

    //QVector<StorageAuditRow> auditRows
    lastAuditRows= StorageAuditService::generateAuditRows(pickingMap);
    view->update_StorageAuditTable(lastAuditRows);
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
    QVector<Cutting::Plan::CutPlan>& plans = model.getResult_PlansRef(); // vagy getMutablePlans()
    const QVector<Cutting::Result::ResultModel> results = model.getResults_Leftovers();

    qDebug() << "✅ VÁGÁSI TERVEK — CutPlan-ek:";
    for (const Cutting::Plan::CutPlan& plan : plans) {
        QStringList pieceLabels, kerfLabels, wasteLabels;

        for (const Cutting::Segment::SegmentModel& s : plan.segments) {
            switch (s.type) {
            case Cutting::Segment::SegmentModel::Type::Piece:  pieceLabels << s.toLabelString(); break;
            case Cutting::Segment::SegmentModel::Type::Kerf:   kerfLabels  << s.toLabelString(); break;
            case Cutting::Segment::SegmentModel::Type::Waste:  wasteLabels << s.toLabelString(); break;
            }
        }

        qDebug().nospace()
            << "  → #" << plan.rodNumber
            << " | PlanId: " << plan.planId
            << " | Forrás: " << (plan.source == Cutting::Plan::Source::Reusable ? "♻️ REUSABLE" : "🧱 STOCK")
            << "\n     Azonosító: " << (plan.usedReusable() ? plan.rodId : plan.materialName())
            << " | Vágások száma: " << plan.cuts.size()
            << " | Kerf: " << plan.kerfTotal << " mm"
            << " | Hulladék: " << plan.waste << " mm"
            << "\n     Darabok: " << pieceLabels.join(" ")
            << "\n     Kerf-ek: " << kerfLabels.join(" ")
            << "\n     Hulladék szakaszok: " << wasteLabels.join(" ");
    }

    qDebug() << "♻️ KELETKEZETT HULLADÉKOK — CutResult-ek:";
    for (const Cutting::Result::ResultModel& result : results) {
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

    for (const Cutting::Plan::CutPlan& plan : plans) {
        totalKerf += plan.kerfTotal;
        totalWaste += plan.waste;
        totalCuts += plan.cuts.size();
        totalSegments += plan.segments.size();

        for (const Cutting::Segment::SegmentModel& s : plan.segments) {
            if (s.type == Cutting::Segment::SegmentModel::Type::Kerf)  kerfSegs++;
            if (s.type == Cutting::Segment::SegmentModel::Type::Waste) wasteSegs++;
        }
    }

    qDebug().nospace() << "📈 Összesítés:\n"
                       << "  Vágások összesen:         " << totalCuts << "\n"
                       << "  Kerf összesen:            " << totalKerf << " mm (" << kerfSegs << " szakasz)\n"
                       << "  Hulladék összesen:        " << totalWaste << " mm (" << wasteSegs << " szakasz)\n"
                       << "  Teljes szakaszszám:       " << totalSegments;

    qDebug() << "***";

    CuttingUtils::logStockStatus("🧱 STOCK — finalize előtt:", StockRegistry::instance().readAll());
    CuttingUtils::logReusableStatus("♻️ REUSABLE — finalize előtt:", LeftoverStockRegistry::instance().readAll());

    // ✂️ Finalizálás → készletfogyás + hulladékkezelés
    CuttingPlanFinalizer::finalize(plans, results);

    qDebug() << "***";

    CuttingUtils::logStockStatus("🧱 STOCK — finalize után:", StockRegistry::instance().readAll());
    CuttingUtils::logReusableStatus("♻️ REUSABLE — finalize után:", LeftoverStockRegistry::instance().readAll());

    // ✅ Állapot lezárása
    for (Cutting::Plan::CutPlan& plan : model.getResult_PlansRef())
        plan.setStatus(Cutting::Plan::Status::Completed);

    // 🔁 View frissítése
    if (view) {
        view->refresh_StockTable();
        // todo 02 : nem jó, nem a táblát kellene frissíteni, hanem a stockot
        view->refresh_LeftoversTable();//CutResultUtils::toReusableEntries(results));
        view->update_ResultsTable(plans);
    }
}

void CuttingPresenter::scrapShortLeftovers()
{
    auto& reusableRegistry = LeftoverStockRegistry::instance();
    QVector<ArchivedWasteEntry> archivedEntries;
    QVector<LeftoverStockEntry> toBeScrapped;

    for (const LeftoverStockEntry &entry : reusableRegistry.readAll()) {
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
        reusableRegistry.consumeEntry(e.barcode);

    if (!archivedEntries.isEmpty())
        ArchivedWasteUtils::exportToCSV(archivedEntries);
}

void CuttingPresenter::syncModelWithRegistries() {
    auto requestList  = CuttingPlanRequestRegistry::instance().readAll();
    auto stockList    = StockRegistry::instance().readAll();
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

bool CuttingPresenter::loadCuttingPlanFromFile(const QString& path) {
    bool loaded = CuttingRequestRepository::loadFromFile(CuttingPlanRequestRegistry::instance(), path);

    return loaded;
}

/*StorageAudit*/


void CuttingPresenter::runStorageAudit(const QMap<QString, int>& pickingMap) {
    QVector<StorageAuditRow> entries = StorageAuditService::generateAuditRows(pickingMap);

    if (view) {
        view->update_StorageAuditTable(entries); // 📋 Audit tábla frissítése
    }

    // opcionális: export, log, statisztika
}

/*PickingPlan*/

QMap<QString, int> CuttingPresenter::generatePickingMapFromPlans(const QVector<Cutting::Plan::CutPlan>& plans) {
    QMap<QString, int> pickingMap;

    for (const auto& plan : plans) {
        for (const auto& cut : plan.cuts) {
            auto mid = cut.materialId;
            auto* mat = MaterialRegistry::instance().findById(mid);
            if(mat){
                QString barcode = mat->barcode;
                int quantity = 1; // Minden darab egy egység – ha van külön mennyiség mező, azt használd

                pickingMap[barcode] += quantity;
            }

        }
    }

    return pickingMap;
}

/*Relocation*/

QVector<RelocationInstruction> CuttingPresenter::generateRelocationPlan(
    const QVector<StorageAuditRow>& auditRows,
    const QString& cuttingZoneName1
    )
{
    QVector<RelocationInstruction> plan;

    // 1. Végigmegyünk minden audit soron, ami a cutting zónához tartozik
    for (const auto& row : auditRows) {
        /*if (row.storageName != cuttingZoneName)
            continue; // csak a célzónában nézzük a hiányt
*/
        int needToMove = row.missingQuantity();
        if (needToMove <= 0)
            continue; // nincs mit odavinni

        // 2. Keressünk forráshelyet ugyanarra a barcode-ra
        for (const auto& sourceRow : auditRows) {
            if (sourceRow.materialBarcode == row.materialBarcode &&
              //  sourceRow.storageName != cuttingZoneName &&
                sourceRow.actualQuantity > 0)
            {
                int moveQty = qMin(needToMove, sourceRow.actualQuantity);

                plan.push_back({
                    row.materialBarcode,     // anyag azonosító
                    sourceRow.storageName,   // honnan
                    //cuttingZoneName,         // hova
                    row.storageName,         // hova
                    moveQty                  // mennyit
                });

                needToMove -= moveQty;
                if (needToMove <= 0)
                    break; // már betelt a hiány
            }
        }
    }

    return plan;
}
