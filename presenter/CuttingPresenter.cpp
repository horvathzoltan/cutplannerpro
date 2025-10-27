#include "CuttingPresenter.h"
#include "../view/MainWindow.h"

//#include "service/cutting/result/resultutils.h"
#include "service/storageaudit/auditsyncguard.h"
#include "service/storageaudit/auditutils.h"
#include "common/logger.h"
//#include "model/registries/materialregistry.h"
//#include "model/relocation/relocationinstruction.h"
#include "model/storageaudit/storageauditentry.h"
#include "service/cutting/result/archivedwasteutils.h"
//#include "service/cutting/result/resultutils.h"
//#include "model/archivedwasteentry.h"
#include "model/registries/cuttingplanrequestregistry.h"
#include "model/registries/leftoverstockregistry.h"
#include "model/registries/stockregistry.h"
//#include "service/cutting/plan/finalizer.h"
#include "service/cutting/optimizer/exporter.h"
//#include "service/cutting/result/archivedwasteutils.h"

#include "common/filenamehelper.h"
#include "common/settingsmanager.h"

#include <model/repositories/cuttingrequestrepository.h>

#include <service/storageaudit/leftoverauditservice.h>
#include <service/storageaudit/storageauditservice.h>

#include <model/registries/materialregistry.h>
#include <model/registries/storageregistry.h>

#include <service/relocation/relocationplanner.h>

#include <service/cutting/optimizer/optimizationauditbuilder.h>
#include <service/cutting/optimizer/optimizationlogger.h>
#include <service/cutting/optimizer/optimizationrunner.h>
#include <service/cutting/optimizer/optimizationviewupdater.h>

#include <service/snapshot/inventorysnapshotbuilder.h>
#include <service/snapshot/requestsnapshotbuilder.h>

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
void CuttingPresenter::add_StockEntry(const StockEntry& entry) {
    StockRegistry::instance().registerEntry(entry);
    if(view){
        view->addRow_StockTable(entry);
    }
    _auditStateManager.setOutdated(AuditStateManager::AuditOutdatedReason::StockChanged);
}

void CuttingPresenter::remove_StockEntry(const QUuid& stockId) {
    StockRegistry::instance().removeEntry(stockId);   // ✅ Globális törlés
    if (view) {
        view->removeRow_StockTable(stockId); // ha a készlet változik
    }
    _auditStateManager.notifyStockRemoved(stockId);
}

void CuttingPresenter::update_StockEntry(const StockEntry& updated) {
    bool ok = StockRegistry::instance().updateEntry(updated); // 🔁 adatbázis update

    if (ok){
        if(view){
            view->updateRow_StockTable(updated);
        }
        _auditStateManager.notifyStockChanged(updated);
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
    _auditStateManager.setOutdated(AuditStateManager::AuditOutdatedReason::LeftoverChanged);
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

// void CuttingPresenter::setStockInventory(const QVector<StockEntry>& list) {
//     model.setStockInventory(list);
// }

// void CuttingPresenter::setReusableInventory(const QVector<LeftoverStockEntry>& list) {
//     model.setReusableInventory(list);
// }

// void CuttingPresenter::setKerf(int kerf) {
//     model.setKerf(kerf);
// }

QVector<Cutting::Plan::CutPlan>& CuttingPresenter::getPlansRef()
{
    return model.getResult_PlansRef();
}

QVector<Cutting::Result::ResultModel> CuttingPresenter::getLeftoverResults()
{
    return model.getResults_Leftovers();
}

// void CuttingPresenter::runOptimization(Cutting::Optimizer::TargetHeuristic heuristic) {
//     // 🔒 Ellenőrzés: a modell szinkronizálva van-e a legfrissebb adatokkal
//     if (!isModelSynced) {
//         zWarning(L("⚠️ A modell nem volt szinkronizálva optimalizáció előtt!"));
//         // Itt opcionálisan vissza is térhetnénk, vagy automatikusan szinkronizálhatnánk
//         return;
//     }

//     // 🚀 Optimalizáció futtatása a modellben
//     model.optimize(heuristic);
//     isModelSynced = false; // újra false, hogy ha később újra hívják, akkor ismét szinkron kelljen

//     // 📋 Optimalizációs tervek logolása (debug célokra)
//     logPlans();

//     // 📦 Az optimalizáció eredményeként létrejött vágási tervek
//     // Minden CutPlan egy konkrét rúd (stock vagy hulló) felhasználását írja le
//     QVector<Cutting::Plan::CutPlan> &plans = model.getResult_PlansRef();

//     // ✨ UI frissítése, ha van aktív nézet
//     if (view) {
//         // Középső eredménytábla frissítése a vágási tervekkel
//         view->update_ResultsTable(plans);

//         // Készlet tábla frissítése (ha a készlet változik az optimalizáció hatására)
//         view->refresh_StockTable();

//         // Hullók kigyűjtése és konvertálása újrafelhasználható bejegyzésekké
//         QVector<Cutting::Result::ResultModel> l = model.getResults_Leftovers();
//         QVector<LeftoverStockEntry> e = Cutting::Result::ResultUtils::toReusableEntries(l);

//         // ⚠️ TODO: itt jelenleg nem generálunk külön leftover audit sorokat,
//         // mert a finalize lépés fogja ténylegesen frissíteni a stockot.
//         // Ezért most csak a táblát frissítjük.
//         view->refresh_LeftoversTable(); // paraméter nélkül, csak vizuális frissítés
//     }

//     // 📤 Export: optimalizációs tervek mentése CSV és TXT formátumban
//     OptimizationExporter::exportPlansToCSV(plans);
//     OptimizationExporter::exportPlansAsWorkSheetTXT(plans);

//     // 📊 Statisztikák frissítése a nézetben
//     view->updateStats(plans, model.getResults_Leftovers());

//     // 🗺️ PickingMap generálása: anyag → hány rúd kell az optimalizációhoz
//     QMap<QUuid, int> pickingMap = generatePickingMapFromPlans(plans);
//     for (auto it = pickingMap.begin(); it != pickingMap.end(); ++it) {
//         QString msg = L("📦 Picking: %1 -> %2").arg(it.key().toString()).arg(it.value());
//         zInfo(msg); // logoljuk, hogy melyik anyagból mennyi kell
//     }

//     // 📥 Audit sorok legenerálása a teljes stockból és a hullókból
//     QVector<StorageAuditRow> stockAuditRows =
//         StorageAuditService::generateAuditRows_All();
//     QVector<StorageAuditRow> leftoverAuditRows =
//         LeftoverAuditService::generateAuditRows_All();

//     // Egyesített audit sor lista (stock + leftover)
//     lastAuditRows = stockAuditRows + leftoverAuditRows;

//     // 🧩 A vágási tervek injektálása az audit sorokba:
//     // - beállítja a pickingQuantity-t (elvárt mennyiség)
//     // - jelöli, hogy a sor része-e az optimalizációnak
//     // - presence státuszt is frissíti (Present/Missing)
//     AuditUtils::injectPlansIntoAuditRows(plans, &lastAuditRows);

//     // 🔗 Kontextus építése a planből származó igényekkel
//     AuditUtils::assignContextsToRows(&lastAuditRows, pickingMap);
//     // 🔗 Kontextus építése: anyag+hely szinten összesítjük az elvárt és tényleges mennyiségeket

//     // auto contextMap = AuditContextBuilder::buildFromRows(lastAuditRows);
//     // for (auto& row : lastAuditRows) {
//     //     row.context = contextMap.value(row.rowId); // minden sor kap egy context pointert
//     // }



//     // 🖥️ Végül frissítjük az Audit táblát a nézetben
//     view->update_StorageAuditTable(lastAuditRows);
// }
void CuttingPresenter::runOptimization(Cutting::Optimizer::TargetHeuristic heuristic) {
    if (!isModelSynced) {
        zWarning(L("⚠️ Modell nincs szinkronizálva optimalizáció előtt!"));
        return;
    }

    // 1️⃣ Optimalizáció futtatása
    OptimizationRunner::run(model, heuristic);

    // 2️⃣ Nézet frissítése
    if (view) {
        OptimizationViewUpdater::update(view, model);
    }

    // 3️⃣ Export (opcionális)
    OptimizationExporter::exportPlans(model.getResult_PlansRef());

    // 4️⃣ Audit sorok előállítása
    _auditStateManager.setOutdated(AuditStateManager::AuditOutdatedReason::OptimizeRun);
    lastAuditRows = OptimizationAuditBuilder::build(model);

    // 5️⃣ Logolás
    OptimizationLogger::logPlans(model.getResult_PlansRef(), model.getResults_Leftovers());

    isModelSynced = false;
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

void CuttingPresenter::logPlans(){
    QVector<Cutting::Plan::CutPlan>& plans = model.getResult_PlansRef(); // vagy getMutablePlans()
    const QVector<Cutting::Result::ResultModel> results = model.getResults_Leftovers();

    zInfo(L("✅ VÁGÁSI TERVEK — CutPlan-ek:"));

    // for (const auto& plan : plans) {
    //     qDebug() << "✅ VÁGÁSI TERV #" << plan.rodNumber
    //              << "| Anyag:" << plan.materialId
    //              << "| Hossz:" << plan.totalLength << "mm"
    //              << "| Hulló:" << (plan.source == Cutting::Plan::Source::Reusable ? "Igen" : "Nem")
    //              << "| Veszteség:" << plan.waste << "mm"
    //              << "| Vágások száma:" << plan.piecesWithMaterial.size();

    //     for (const auto& cut : plan.piecesWithMaterial) {
    //         const auto& info = cut.info;
    //         qDebug() << "  ✂️ Darab:" << info.length_mm << "mm"
    //                  << "| Megrendelő:" << info.ownerName
    //                  << "| Tételszám:" << info.externalReference
    //                  << "| Kérelem anyag:" << cut.materialId;
    //     }

    //     qDebug() << "--------------------------------------------";
    // }

    for (const Cutting::Plan::CutPlan& plan : plans) {
        QStringList pieceLabels, kerfLabels, wasteLabels;

        for (const Cutting::Segment::SegmentModel& s : plan.segments) {
            switch (s.type) {
            case Cutting::Segment::SegmentModel::Type::Piece:  pieceLabels << s.toLabelString(); break;
            case Cutting::Segment::SegmentModel::Type::Kerf:   kerfLabels  << s.toLabelString(); break;
            case Cutting::Segment::SegmentModel::Type::Waste:  wasteLabels << s.toLabelString(); break;
            }
        }

        QString msg =
            L("  → %1").arg(plan.rodId) +
            L(" | PlanId: %1").arg(plan.planId.toString()) +
            L(" | Forrás: %1").arg(plan.source == Cutting::Plan::Source::Reusable ? "♻️ REUSABLE" : "🧱 STOCK") +
            L("\n     Azonosító: %1").arg(plan.isReusable() ? plan.rodId : plan.materialName()) +
            L(" | Vágások száma: %1").arg(plan.piecesWithMaterial.size()) +
            L(" | Kerf: %1 mm").arg(plan.kerfTotal) +
            L(" | Hulladék: %1 mm").arg(plan.waste) +
            L("\n     Darabok: %1").arg(pieceLabels.join(" ")) +
            L("\n     Kerf-ek: %1").arg(kerfLabels.join(" ")) +
            L("\n     Hulladék szakaszok: %1").arg(wasteLabels.join(" "));

        zInfo(msg);
    }

    zInfo(L("♻️ KELETKEZETT HULLADÉKOK — CutResult-ek:"));

    for (const Cutting::Result::ResultModel& result : results) {
        QString msg =
            L("  - Hulladék: %1 mm").arg(result.waste) +
            L(" | Forrás: %1").arg(result.sourceAsString()) +
            L(" | MaterialId: %1").arg(result.materialId.toString()) +
            L(" | Barcode: %1").arg(result.reusableBarcode) +
            L("\n    Darabok: %1").arg(result.cutsAsString());

        zInfo(msg);
    }


    // 📊 Összesítés
    int totalKerf = 0, totalWaste = 0, totalCuts = 0;
    int totalSegments = 0, kerfSegs = 0, wasteSegs = 0;

    for (const Cutting::Plan::CutPlan& plan : plans) {
        totalKerf += plan.kerfTotal;
        totalWaste += plan.waste;
        totalCuts += plan.piecesWithMaterial.size();
        totalSegments += plan.segments.size();

        for (const Cutting::Segment::SegmentModel& s : plan.segments) {
            if (s.type == Cutting::Segment::SegmentModel::Type::Kerf)  kerfSegs++;
            if (s.type == Cutting::Segment::SegmentModel::Type::Waste) wasteSegs++;
        }
    }

    QString msg =
        L("📈 Összesítés:\n") +
        L("  Vágások összesen:         %1").arg(totalCuts) + "\n" +
        L("  Kerf összesen:            %1 mm (%2 szakasz)").arg(totalKerf).arg(kerfSegs) + "\n" +
        L("  Hulladék összesen:        %1 mm (%2 szakasz)").arg(totalWaste).arg(wasteSegs) + "\n" +
        L("  Teljes szakaszszám:       %1").arg(totalSegments);

    zInfo(msg);

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
    // 🔁 Snapshotok építése a registrykből
    auto requests  = RequestSnapshotBuilder::build();
    auto inventory = InventorySnapshotBuilder::build(300);

    QStringList errors;

    // 📋 Validáció
    if (requests.isEmpty())
        errors << "Nincs megadva vágási igény.";

    if (inventory.profileInventory.isEmpty())
        errors << "A készlet üres.";

    if (inventory.reusableInventory.isEmpty())
        errors << "Nincs újrahasználható hulladék elérhető.";

    // ✅ Modell feltöltése vagy ❗ hibaüzenet
    if (errors.isEmpty()) {
        model.setCuttingRequests(requests);
        model.setInventorySnapshot(inventory);   // << pengeéles snapshot betöltés
        isModelSynced = true;
    } else {
        QString fullMessage = "Az optimalizálás nem indítható:\n\n• " + errors.join("\n• ");
        if (view)
            view->ShowWarningDialog(fullMessage);
        isModelSynced = false;
    }
}

bool CuttingPresenter::loadCuttingPlanFromFile(const QString& path) {
    bool loaded = CuttingRequestRepository::loadFromFile(CuttingPlanRequestRegistry::instance(), path);

    return loaded;
}

/*StorageAudit*/


void CuttingPresenter::runStorageAudit() {
    QVector<StorageAuditRow> stockAuditRows =
        StorageAuditService::generateAuditRows_All();
    QVector<StorageAuditRow> leftoverAuditRows =
        LeftoverAuditService::generateAuditRows_All();

    lastAuditRows = stockAuditRows + leftoverAuditRows;

    // 🔁 Ha van optimalizációs terv, injektáljuk vissza
    if (!model.getResult_PlansRef().isEmpty()) {
        QVector<Cutting::Plan::CutPlan>& plans = model.getResult_PlansRef();
        AuditUtils::injectPlansIntoAuditRows(plans, &lastAuditRows);

        QMap<QUuid, int> pickingMap = generatePickingMapFromPlans(plans);
        AuditUtils::assignContextsToRows(&lastAuditRows, pickingMap);
    } else {
        AuditUtils::assignContextsToRows(&lastAuditRows, {});
    }

    if (view) {
        view->update_StorageAuditTable(lastAuditRows);
    }

    _auditStateManager.setActiveAuditRows(lastAuditRows);
}

/*PickingPlan*/

// a requestben van egy material - ebből az anyagból szeretnénk levágni - requestMaterial
// a planban van egy material - selectedMaterialId - ezt nem tudjuk hogy mi.
// De: először a reqMaterial groupjában keresünk hullót
// és ha nincsen, akkor a groupjából keresünk egy szálat
// majd elkészülnek a cutok per plan
// minden cutban van egy material, ez a reqMaterial

QMap<QUuid, int> CuttingPresenter::generatePickingMapFromPlans(const QVector<Cutting::Plan::CutPlan>& plans) {
    QMap<QUuid, int> pickingMap;

    for (const auto& plan : plans) {
        if(plan.isReusable())
            continue; // csak a stockból vágott anyagok számítanak

        pickingMap[plan.materialId] += 1; // minden CutPlan egy rúd
        /*auto groupIds = GroupUtils::groupMembers(plan.materialId);
        for (const auto& gid : groupIds) {
            pickingMap[gid] += 1;
        }*/

    }

    return pickingMap;
}


// QVector<QString> CuttingPresenter::resolveTargetStorages(const QUuid& rootStorageId) {
//     QVector<QString> result;

//     // Root maga
//     if (const auto* root = StorageRegistry::instance().findById(rootStorageId)) {
//         result.append(root->name);
//     }

//     // Gyerekek rekurzívan
//     std::function<void(const QUuid)> collectChildren = [&](const QUuid parentId) {
//         const auto children = StorageRegistry::instance().findByParentId(parentId);
//         for (const auto& child : children) {
//             result.append(child.name);
//             collectChildren(child.id); // mélyebb szintek
//         }
//     };

//     collectChildren(rootStorageId);
//     return result;
// }

void CuttingPresenter::updateRow(const QUuid& rowId,
                                 std::function<void(StorageAuditRow&)> updater)
{
    for (StorageAuditRow& row : lastAuditRows) {
        if (row.rowId == rowId) {
            updater(row);
            if (view) {
                view->updateRow_StorageAuditTable(row);
            }
            break;
        }
    }
}


void CuttingPresenter::update_StorageAuditActualQuantity(const QUuid& rowId, int actualQuantity)
{
    AuditSyncGuard guard(&_auditStateManager);

    updateRow(rowId, [&](StorageAuditRow& row) {
        row.actualQuantity = actualQuantity;
        row.isRowModified = (actualQuantity != row.originalQuantity);

        // 🔄 Stock frissítés
        if (auto opt = StockRegistry::instance().findById(row.stockEntryId); opt.has_value()) {
            StockEntry updated = opt.value();
            updated.quantity = actualQuantity;
            StockRegistry::instance().updateEntry(updated);

            if (view) {
                view->updateRow_StockTable(updated);
            }
        }
    });
}

void CuttingPresenter::update_StorageAuditCheckbox(const QUuid& rowId, bool checked)
{
    updateRow(rowId, [&](StorageAuditRow& row) {
        row.isRowAuditChecked = checked;
    });
}


void CuttingPresenter::update_LeftoverAuditActualQuantity(const QUuid& rowId, int quantity)
{
    updateRow(rowId, [&](StorageAuditRow& row) {
        if (row.sourceType != AuditSourceType::Leftover) return;

        row.actualQuantity = quantity;
        row.isRowModified = (row.actualQuantity != row.originalQuantity);
    });
}


/*relocation*/


