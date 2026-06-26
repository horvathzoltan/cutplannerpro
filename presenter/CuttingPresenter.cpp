#include "CuttingPresenter.h"
#include "../view/MainWindow.h"

//#include "service/cutting/result/resultutils.h"
#include "../service/storageaudit/auditsyncguard.h"
#include "../service/storageaudit/auditutils.h"
#include "../common/logger.h"
//#include "model/registries/materialregistry.h"
//#include "model/relocation/relocationinstruction.h"
#include "../model/storageaudit/storageauditentry.h"
#include "../service/cutting/result/archivedwasteutils.h"
//#include "service/cutting/result/resultutils.h"
//#include "model/archivedwasteentry.h"
#include "../model/registries/cuttingplanrequestregistry.h"
#include "../model/registries/leftoverstockregistry.h"
#include "../model/registries/stockregistry.h"
//#include "service/cutting/plan/finalizer.h"
#include "../service/cutting/optimizer/exporter.h"
//#include "service/cutting/result/archivedwasteutils.h"

#include "../common/filenamehelper.h"
#include "model/cutting/plan/audit/naphalo_prefix_match.h"
#include "model/cutting/plan/audit/naphalo_profile_postfix.h"
#include "model/cutting/plan/audit/naphalo_type_detector.h"
#include "settings/settingsmanager.h"

#include "../model/repositories/cuttingrequestrepository.h"

#include "../service/storageaudit/leftoverauditservice.h"
#include "../service/storageaudit/storageauditservice.h"

#include "common/eventlogger.h"
#include "materials/registry/material_registry.h"
#include "../model/registries/storageregistry.h"

#include "../service/relocation/relocationplanner.h"

#include "../service/cutting/optimizer/optimizationauditbuilder.h"
#include "../service/cutting/optimizer/optimizationlogger.h"
#include "../service/cutting/optimizer/optimizationrunner.h"
#include "../service/cutting/optimizer/optimizationviewupdater.h"

#include "../service/snapshot/inventorysnapshotbuilder.h"
#include "../service/snapshot/requestsnapshotbuilder.h"
#include "materials/utils/material_group_utils.h"
#include "service/cutting/instruction/cuttinginstructionutils.h"
#include "service/cutting/summary/cutplansummary.h"

#include <service/cutting/summary/cutplansummarybuilder.h>

#include <QDir>
#include <QFileInfo>
#include <QPdfWriter>
#include <ui_clonerequestdialog.h>

#include <model/registries/cuttingmachineregistry.h>

#include <model/repositories/cuttingrequestrepository.h>

#include <model/cutting/plan/audit/naphalo_audit_service.h>

#include <model/cutting/plan/audit/naphalo_audit_types.h>

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

bool CuttingPresenter::remove_LeftoverStockEntry(const QUuid& entryId) {
    bool ok = LeftoverStockRegistry::instance().removeEntry(entryId);

    if(!ok){
        qWarning() << "❌ Sikertelen törlés: nincs ilyen entryId:" << entryId;
        return false;
    }

    if(view){
        view->removeRow_LeftoversTable(entryId);
    }
    return true;
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

        // 🔧 Hulló készlet használatának beállítása
    model.setUseReusableLeftovers(
        view->isChkUseLeftoversChecked()
        );

    zInfo("▶️ OptimizationRunner::run started");
    // 1️⃣ Optimalizáció futtatása
    OptimizationRunner::run(model, heuristic);
    zInfo("⏹️ OptimizationRunner::run stopped");
    // 5️⃣ Logolás
    OptimizationLogger::logPlans(model.getResult_PlansRef());

    // 2️⃣ Nézet frissítése
    if (view) {
        OptimizationViewUpdater::update(view, model);
        view->switchToCuttingPlanTab();   // ⬅️ EZT ADJUK HOZZÁ
    }

    // 3️⃣ Export (opcionális)
    //OptimizationExporter::exportPlans(model.getResult_PlansRef());

    // 4️⃣ Audit sorok előállítása
    _auditStateManager.setOutdated(AuditStateManager::AuditOutdatedReason::OptimizeRun);
    //lastAuditRows = OptimizationAuditBuilder::build(model);


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

// void CuttingPresenter::logPlans(){
//     QVector<Cutting::Plan::CutPlan>& plans = model.getResult_PlansRef(); // vagy getMutablePlans()
//     const QVector<Cutting::Result::ResultModel> results = model.getResults_Leftovers();

//     zInfo(L("✅ VÁGÁSI TERVEK — CutPlan-ek:"));

//     // for (const auto& plan : plans) {
//     //     qDebug() << "✅ VÁGÁSI TERV #" << plan.rodNumber
//     //              << "| Anyag:" << plan.materialId
//     //              << "| Hossz:" << plan.totalLength << "mm"
//     //              << "| Hulló:" << (plan.source == Cutting::Plan::Source::Reusable ? "Igen" : "Nem")
//     //              << "| Veszteség:" << plan.waste << "mm"
//     //              << "| Vágások száma:" << plan.piecesWithMaterial.size();

//     //     for (const auto& cut : plan.piecesWithMaterial) {
//     //         const auto& info = cut.info;
//     //         qDebug() << "  ✂️ Darab:" << info.length_mm << "mm"
//     //                  << "| Megrendelő:" << info.ownerName
//     //                  << "| Tételszám:" << info.externalReference
//     //                  << "| Kérelem anyag:" << cut.materialId;
//     //     }

//     //     qDebug() << "--------------------------------------------";
//     // }

//     for (const Cutting::Plan::CutPlan& plan : plans) {
//         QStringList pieceLabels, kerfLabels, wasteLabels;

//         for (const Cutting::Segment::SegmentModel& s : plan.segments) {
//             if (s.isPiece()) {
//                 pieceLabels << s.toLabelString();
//             }
//             else if (s.isKerf()) {
//                 kerfLabels << s.toLabelString();
//             }
//             else if (s.isWaste()) {
//                 wasteLabels << s.toLabelString();
//             }
//             else if (s.isTechnical()) {
//                 // ha kell külön lista, ide jön
//                 // pl.: technicalLabels << s.toLabelString();
//             }
//         }

//         QStringList segmentstxt;

//         for(const auto& p : plan.piecesWithMaterial) {
//             const MaterialMaster* mat = MaterialRegistry::instance().findById(p.materialId);
//             segmentstxt << QString("%1 %2 mm)").arg(mat?mat->toDisplay():"(?)").arg(p.info.length_mm);
//         }

//         QString msg =
//             L("  → %1").arg(plan.rodId) +
//             L(" | segments: %1").arg(segmentstxt.join(',')) +
//             L(" | Forrás: %1").arg(plan.source == Cutting::Plan::Source::Reusable ? "♻️ REUSABLE" : "🧱 STOCK") +
//             L("\n     Azonosító: %1").arg(plan.isReusable() ? plan.rodId : plan.materialName()) +
//             L(" | Vágások száma: %1").arg(plan.piecesWithMaterial.size()) +
//             L(" | Kerf: %1 mm").arg(plan.kerfTotal) +
//             L(" | Hulladék: %1 mm").arg(plan.waste) +
//             L("\n     Darabok: %1").arg(pieceLabels.join(" ")) +
//             L("\n     Kerf-ek: %1").arg(kerfLabels.join(" ")) +
//             L("\n     Hulladék szakaszok: %1").arg(wasteLabels.join(" "));

//         zInfo(msg);
//     }

//     zInfo(L("♻️ KELETKEZETT HULLADÉKOK — CutResult-ek:"));

//     for (const Cutting::Result::ResultModel& result : results) {
//         QString msg =
//             L("  - Hulladék: %1 mm").arg(result.waste) +
//             L(" | Forrás: %1").arg(result.sourceAsString()) +
//             L(" | MaterialId: %1").arg(result.materialId.toString()) +
//             L(" | Barcode: %1").arg(result.reusableBarcode) +
//             L("\n    Darabok: %1").arg(result.cutsAsString());

//         zInfo(msg);
//     }


//     // 📊 Összesítés
//     int totalKerf = 0, totalWaste = 0, totalCuts = 0;
//     int totalSegments = 0, kerfSegs = 0, wasteSegs = 0;

//     for (const Cutting::Plan::CutPlan& plan : plans) {
//         totalKerf += plan.kerfTotal;
//         totalWaste += plan.waste;
//         totalCuts += plan.piecesWithMaterial.size();
//         totalSegments += plan.segments.size();

//         for (const Cutting::Segment::SegmentModel& s : plan.segments) {
//             if (s.isKerf()) {
//                 kerfSegs++;
//             }
//             else if (s.isWaste()) {
//                 wasteSegs++;
//             }
//         }
//     }

//     QString msg =
//         L("📈 Összesítés:\n") +
//         L("  Vágások összesen:         %1").arg(totalCuts) + "\n" +
//         L("  Kerf összesen:            %1 mm (%2 szakasz)").arg(totalKerf).arg(kerfSegs) + "\n" +
//         L("  Hulladék összesen:        %1 mm (%2 szakasz)").arg(totalWaste).arg(wasteSegs) + "\n" +
//         L("  Teljes szakaszszám:       %1").arg(totalSegments);

//     zInfo(msg);

// }

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

// void CuttingPresenter::syncModelWithRegistries() {
//     // 🔁 Snapshotok építése a registrykből
//     auto requests  = RequestSnapshotBuilder::build();
//     auto inventory = InventorySnapshotBuilder::build(300);

//     QStringList errors;

//     // 📋 Validáció
//     if (requests.isEmpty())
//         errors << "Nincs megadva vágási igény.";

//     if (inventory.profileInventory.isEmpty())
//         errors << "A készlet üres.";

//     // A reusableInventory opcionális, nem kötelező.
//     // Ha üres, az NEM hiba.
//     // if (inventory.reusableInventory.isEmpty())
//     //     errors << "Nincs újrahasználható hulladék elérhető.";

//     // ✅ Modell feltöltése vagy ❗ hibaüzenet
//     if (errors.isEmpty()) {
//         model.setCuttingRequests(requests);
//         model.setInventorySnapshot(inventory);   // << pengeéles snapshot betöltés
//         isModelSynced = true;
//     } else {
//         QString fullMessage = "Az optimalizálás nem indítható:\n\n• " + errors.join("\n• ");
//         if (view)
//             view->ShowWarningDialog(fullMessage);
//         isModelSynced = false;
//     }
// }


void CuttingPresenter::syncModelWithRegistries() {
    auto requests  = RequestSnapshotBuilder::build();
    auto inventory = InventorySnapshotBuilder::build(300);

    // 🔍 DEBUG: request lista kiírása
    zInfo("📦 RequestSnapshotBuilder::build():");
    for (const Cutting::Plan::Request &r : requests) {
        const MaterialMaster* mat = MaterialRegistry::instance().findById(r.materialId);
        zInfo(QString("   •  %1. %2 %3, %4 mm, %5 db ")
                  .arg(r.externalReference)
                  .arg(r.ownerName)
                  .arg(mat?mat->toDisplay():"(?)")
                  .arg(r.requiredLength)
                  .arg(r.quantity));
    }
    zInfo(QString("   → összesen %1 db").arg(requests.size()));

    QStringList errors;
    QStringList warnings;

    // 1️⃣ Legyen legalább 1 request
    if (requests.isEmpty()) {
        errors << "Nincs megadva vágási igény.";
    }

    // 2️⃣ Request anyagok összegyűjtése
    QSet<QUuid> reqMaterials;
    QMap<QUuid, int> reqTotalLength;

    for (const auto& r : requests) {
        reqMaterials.insert(r.materialId);
        reqTotalLength[r.materialId] += r.requiredLength;
    }

    // 3️⃣ Stock anyagok összegyűjtése
    QSet<QUuid> stockMaterials;
    QMap<QUuid, int> stockTotalLength;

    for (const auto& s : inventory.profileInventory) {
        stockMaterials.insert(s.materialId);

        const MaterialMaster* mm = s.master();
        if (mm)
            stockTotalLength[s.materialId] += mm->stockLength_mm * s.quantity;
    }

    // 4️⃣ Hiányzó anyagok (nincs a stockban)
    QSet<QUuid> missingMaterials = reqMaterials - stockMaterials;

    for (const QUuid& matId : missingMaterials) {
        const MaterialMaster* mat = MaterialRegistry::instance().findById(matId);

        if (!mat) {
            errors << QString("Ismeretlen anyag (materialId=%1)").arg(matId.toString());
            continue;
        }

        // 5️⃣ Csoporthelyettesítés vizsgálata
        QSet<QUuid> group = GroupUtils::groupMembers(matId);

        bool hasGroupAlternative = false;
        for (const QUuid& altId : group) {
            if (altId == matId) continue;
            if (stockMaterials.contains(altId)) {
                const MaterialMaster* alt = MaterialRegistry::instance().findById(altId);
                warnings << QString("Az anyag (%1) nincs raktáron, de a csoportban van helyettesítő: %2")
                                .arg(mat->toDisplay())
                                .arg(alt ? alt->toDisplay() : altId.toString());
                hasGroupAlternative = true;
                break;
            }
        }

        if (!hasGroupAlternative) {
            errors << QString("Hiányzó anyag a készletből: %1")
                          .arg(mat->toDisplay());
        }
    }

    // 6️⃣ Mennyiséghiány (összhossz alapján)
    for (auto it = reqTotalLength.begin(); it != reqTotalLength.end(); ++it) {
        QUuid matId = it.key();
        int need = it.value();
        int have = stockTotalLength.value(matId, 0);

        if (have < need) {
            const MaterialMaster* mat = MaterialRegistry::instance().findById(matId);
            QString name = mat ? mat->toDisplay() : matId.toString();

            warnings << QString("Kevés készlet az anyagból: %1 (kell: %2 mm, van: %3 mm)")
                            .arg(name)
                            .arg(need)
                            .arg(have);
        }
    }

    // 7️⃣ Hibák → tiltás
    if (!errors.isEmpty()) {
        QString msg = "Az optimalizálás nem indítható:\n\n• " + errors.join("\n• ");
        if (!warnings.isEmpty())
            msg += "\n\nFigyelmeztetések:\n• " + warnings.join("\n• ");

        if (view)
            view->ShowWarningDialog(msg);

        isModelSynced = false;
        return;
    }

    // 8️⃣ Modell betöltése
    model.setCuttingRequests(requests);
    model.setInventorySnapshot(inventory);
    isModelSynced = true;

    // 9️⃣ Warningok megjelenítése (nem tiltó)
    if (!warnings.isEmpty() && view) {
        QString msg = "Figyelmeztetések:\n\n• " + warnings.join("\n• ");
        view->ShowWarningDialog(msg);
    }

}


bool CuttingPresenter::loadCuttingPlanFromFile(const QString& path) {
    bool loaded = CuttingRequestRepository::loadFromFile(CuttingPlanRequestRegistry::instance(), path);

    return loaded;
}

/*StorageAudit*/
/**
 * @brief Teljes audit futtatása: stock + leftover sorok összegyűjtése és context hozzárendelése.
 *
 * Lépései:
 * 1. Legenerálja a stock audit sorokat a StorageAuditService-ből.
 * 2. Legenerálja a leftover audit sorokat a LeftoverAuditService-ből.
 * 3. Összefűzi a két listát (lastAuditRows).
 * 4. Ha van optimalizációs terv:
 *    - Meghívja az AuditUtils::injectPlansIntoAuditRows függvényt, amely
 *      beállítja a sorok isInOptimization flag-jét és leftover esetén a 0/1 expected értéket.
 *    - Legenerálja a pickingMap-et a generatePickingMapFromPlans segítségével.
 *    - Meghívja az AuditUtils::assignContextsToRows függvényt, amely contextet rendel a sorokhoz.
 * 5. Ha nincs optimalizációs terv, akkor üres pickingMap-pel hívja az assignContextsToRows-t.
 * 6. Frissíti a nézetet (update_StorageAuditTable).
 * 7. Beállítja az aktív audit sorokat az AuditStateManager-ben.
 *
 * Ez a függvény a stock és leftover világot egyesíti, és gondoskodik arról,
 * hogy a leftover sorok példány szinten, a stock sorok pedig aggregáltan jelenjenek meg.
 */


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

/**
 * @brief PickingMap generálása a vágási tervek alapján.
 *
 * Feladata:
 * - Csak a stock forrású terveket veszi figyelembe.
 * - Minden stock CutPlan egy rúdnak számít → növeli a materialId-hoz tartozó darabszámot.
 * - A leftover (Reusable) forrású terveket kihagyja, mert azok példány szinten,
 *   külön audit sorban jelennek meg, és nem aggregálódnak materialId szerint.
 *
 * Eredmény:
 * - QMap<materialId, darabszám>, amelyet az AuditContextBuilder::buildFromRows
 *   használ a stock sorok totalExpected értékének kiszámításához.
 *
 * @param plans Az optimalizációs tervek listája.
 * @return QMap<QUuid,int> materialId → elvárt darabszám.
 */

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


void CuttingPresenter::ExportCutPlanSummary() {

    static const QString errevent = QStringLiteral("❌ Summary export nem hajtható végre. Részletek a logban.");
    static const QString oklog = QStringLiteral("✅ Cut Plan Summary exportálva: %1");

    const auto& plans = model.getResult_PlansRef();

    // 1️⃣ Guard: nincs optimalizációs eredmény
    if (plans.isEmpty()) {
        if (view)
            view->ShowWarningDialog(
                "Nincs optimalizációs eredmény.\n"
                "A Summary export nem hajtható végre."
                );
        return;
    }

    const auto& leftovers = model.getResults_Leftovers();

    QString fileName = SettingsManager::instance().cuttingPlanFileName();
    QFileInfo fi(fileName);
    QString baseName = fi.completeBaseName();

    if (baseName.isEmpty()) {
        zWarning(errevent);
        zEvent("❌ Nincs Cutting Plan fájlnév — a Summary export nem hajtható végre.");
        return;
    }

    CutPlanSummary summary = CutPlanSummaryBuilder::build(plans, leftovers, baseName, model._fitTelemetry);

    QString dir = fi.absolutePath() + "/_reports";
    QDir().mkpath(dir);  // ha nincs, létrehozzuk
    QString path = dir + "/" + baseName + "_CutPlanSummary.txt";

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)){
        zWarning(errevent);
        zEvent(QString("❌ Nem sikerült megnyitni a fájlt írásra: %1").arg(path));
        return;
    }


    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8);
    out << summary.toText() << "\n";

    file.close();

    zInfo(oklog.arg(QDir::toNativeSeparators(path)));
    zEvent(oklog.arg(QDir::toNativeSeparators(path)));
}


void CuttingPresenter::GenerateCutInstructions()
{

    auto& cutPlans = model.getResult_PlansRef();

    if (cutPlans.isEmpty()) {
        if (view)
            view->ShowWarningDialog(
                "Nincs optimalizációs eredmény.\n"
                "Előbb futtasd az Optimize műveletet."
                );
        return;
    }

    _machineCutsList.clear();

    //auto& cutPlans = model.getResult_PlansRef();
    auto leftovers = model.getResults_Leftovers();

    QHash<QUuid,int> requestPieceCounters;
    QSet<QString> reusedLeftovers;

    for (const auto& plan : cutPlans)
        if (!plan.sourceBarcode.isEmpty())
            reusedLeftovers.insert(plan.sourceBarcode);

    int globalStep = 1;

    for (const auto& plan : cutPlans) {

        const CuttingMachine* machine =
            CuttingMachineRegistry::instance().findById(plan.machineId);
        if (!machine) continue;

        // gép-blokk keresése vagy létrehozása
        auto it = std::find_if(_machineCutsList.begin(), _machineCutsList.end(),
                               [&](const MachineCuts& mc){ return mc.machineHeader.machineId == plan.machineId; });

        if (it == _machineCutsList.end()) {
            MachineCuts mc;
            mc.machineHeader.machineId = plan.machineId;
            mc.machineHeader.machineName = machine->name;
            mc.machineHeader.comment = machine->comment;
            mc.machineHeader.kerf_mm = machine->kerf_mm;
            mc.machineHeader.stellerMaxLength_mm = machine->stellerMaxLength_mm;
            mc.machineHeader.stellerCompensation_mm = machine->stellerCompensation_mm;
            _machineCutsList.push_back(std::move(mc));
            it = _machineCutsList.end() - 1;
        }

        // utolsó piece index
        //int lastPieceIdx = -1;
        // for (int j = plan._segments.size() - 1; j >= 0; --j){
        //     if (plan._segments.segment(j).isPiece())
        //     {
        //         lastPieceIdx = j; break;
        //     }
        // }

        double remaining = plan._segments.totalLength_mm();

        for (int i = 0; i < plan._segments.size(); ++i) {
            const auto& seg = plan._segments.segment(i);
            if (!seg.isPiece()) continue;

            CutInstruction ci;
            ci.globalStepId = globalStep++;
            ci.rodId = plan.rodId;
            ci.materialId = plan.materialId;
            ci.barcode = plan.sourceBarcode;
            ci.cutSize_mm = seg.length_mm();
            ci.kerf_mm = machine->kerf_mm;
            ci.lengthBefore_mm = remaining;
            ci.computeRemaining();
            ci.requestId = seg._requestId;
            ci.status = CutStatus::Pending;
            ci.leftoverBarcode = plan._segments.leftoverBarcode();

            ci.pieceCounter = ++requestPieceCounters[seg._requestId];
            ci.externalReference = seg.externalReference;

            ci.source = plan.source;
            ci.sourceBarcode = plan.sourceBarcode;

            auto pwm = plan.getPieceMaterialBy_pieceId(seg._pieceId);
            ci.side = pwm.side;
            ci.subtype = pwm.subtype;

            // if (i == lastPieceIdx && ci.lengthAfter_mm > 0)
            //     if (!reusedLeftovers.contains(plan.leftoverBarcode))
            //         ci.isFinalLeftover = true;

            it->cutInstructions.push_back(ci);
            remaining = ci.lengthAfter_mm;
        }
    }

    // utófeldolgozás
    for (auto& mc : _machineCutsList)
        CuttingInstructionUtils::postProcessMachineCuts(mc);

    // UI frissítés
    if (view){
        view->renderCuttingInstructions(_machineCutsList);
        view->switchToInstructionsPlanTab();   // ⬅️ EZT ADJUK HOZZÁ

    }
}


void CuttingPresenter::ExportCutInstructions()
{

    if (_machineCutsList.isEmpty()) {
        if (view)
            view->ShowWarningDialog("Nincs legenerált vágási utasítás.\nElőbb futtasd a Generate CutInstructions műveletet.");
        return;
    }

    QString fileName = SettingsManager::instance().cuttingPlanFileName();
    QFileInfo fi(fileName);
    QString baseName = fi.completeBaseName();

    if (baseName.isEmpty()) {
        zEvent("❌ Nincs Cutting Plan fájlnév — export nem lehetséges.");
        return;
    }

    QString dir = fi.absolutePath() + "/_reports";
    QDir().mkpath(dir);

    QString dateStr = QDateTime::currentDateTime().toString("yyyy.MM.dd HH:mm");

    // --- 1) CutInstructions.txt ---
    {
        QString path = dir + "/" + baseName + "_CutInstructions.txt";
        QFile f(path);
        if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
            zEvent("❌ Nem sikerült megnyitni a CutInstructions fájlt.");
            return;
        }

        QTextStream out(&f);
        out.setEncoding(QStringConverter::Utf8);

        for (const auto& mc : _machineCutsList) {
            out << CuttingInstructionUtils::formatMachineCutsEvent(mc, baseName, printedLineWidth) << "\n\n";
        }

        zEvent(QString("📄 CutInstructions exportálva: %1").arg(path));
    }

    // --- 2) LabelTable ---
    {
        QString path = dir + "/" + baseName + "_CutInstructions_Labels.txt";
        QFile f(path);
        if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
            zEvent("❌ Nem sikerült megnyitni a LabelTable fájlt.");
            return;
        }

        QTextStream out(&f);
        out.setEncoding(QStringConverter::Utf8);

        out << QString("🏷️ Címketáblák (gépenként)");
        out << QString("CutPlan: %1\n").arg(baseName);
        out << QString("📅 Dátum: %1\n\n").arg(dateStr);

        bool firstPage = true;

        for (const auto& mc : _machineCutsList) {
            if (!firstPage)
                out << "\n\n";

            firstPage = false;


            out << QString("🪚 Gép: %1\n").arg(mc.machineHeader.machineName);

            auto labels = CuttingInstructionUtils::collectLabelModelsFromMachineCuts(mc);
            //out << CuttingInstructionUtils::formatLabelTable4(labels, printedLineWidth, 2, printedPageHeight);
            //out << CuttingInstructionUtils::formatLabelStackPaged(labels, printedLineWidth, 2, printedPageHeight);
            out << CuttingInstructionUtils::formatLabelColumnFlow(labels, printedLineWidth, printedPageHeight, 2, 4);
            //out << "\n\n";
        }

        zEvent(QString("🏷️ LabelTable exportálva: %1").arg(path));
    }

    // --- 3) LabelTable PDF ---

    {
        QString path = dir + "/" + baseName + "_CutInstructions_Labels.pdf";

        QPdfWriter writer(path);
        writer.setPageSize(QPageSize(QPageSize::A4));
        writer.setResolution(300);

        QPainter painter(&writer);
        if (!painter.isActive()) {
            zEvent("❌ Nem sikerült megnyitni a PDF fájlt.");
            return;
        }

        QRectF pageRect = writer.pageLayout().paintRectPixels(writer.resolution());

        const int cols = 2;
        const qreal cellHeight = 240.0; // nagy, jól olvasható címke

        // MONOSPACED FONT – kötelező a TXT‑s spacinghez
        QFont font("Noto Sans Mono", 11);//Noto Sans Mono
        //QFont font("Courier New", 11);
        painter.setFont(font);

        bool firstPage = true;

        for (const auto& mc : _machineCutsList)
        {
            if (!firstPage)
                writer.newPage();   // új lap csak a második géptől

            firstPage = false;

            QVector<LabelModel> labels =
                CuttingInstructionUtils::collectLabelModelsFromMachineCuts(mc);

            CuttingInstructionUtils::formatLabelColumnFlow_Pdf(
                labels,
                painter,
                writer,
                pageRect,
                cols,
                cellHeight
                );
        }

        painter.end();
        zEvent(QString("🏷️ LabelTable PDF exportálva: %1").arg(path));
    }

}


void CuttingPresenter::ExportLeftoverIntakeForm()
{
    int rowsPerPage = 12; // tetszőleges, később paraméterezhető


    QString fileName = SettingsManager::instance().cuttingPlanFileName();
    QFileInfo fi(fileName);
    QString baseName = fi.completeBaseName();

    if (baseName.isEmpty()) {
        zEvent("❌ Nincs Cutting Plan fájlnév — leftover űrlap export nem lehetséges.");
        return;
    }

    QString dir = fi.absolutePath() + "/_reports";
    QDir().mkpath(dir);

    QString dateStr = QDateTime::currentDateTime().toString("yyyy.MM.dd HH:mm");

    int start = SettingsManager::instance().peekManualLeftoverCounter();
    int end   = start + rowsPerPage - 1;

    QString path = QString("%1/leftoverintakeform_RSM-%2-%3.txt")
                       .arg(dir)
                       .arg(start, 3, 10, QChar('0'))
                       .arg(end,   3, 10, QChar('0'));

    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        zEvent("❌ Nem sikerült megnyitni a LeftoverIntakeForm fájlt.");
        return;
    }

    QTextStream out(&f);
    out.setEncoding(QStringConverter::Utf8);

    // 1 lapnyi leftover intake form
    out << CuttingInstructionUtils::formatLeftoverIntakeForm_OnePage(
        printedLineWidth, rowsPerPage);

    zEvent(QString("📄 Leftover Intake Form exportálva: %1").arg(path));
}


void CuttingPresenter::UpdateCompensation(const QUuid& machineId, double newVal)
{
    for (auto& mc : _machineCutsList) {
        if (mc.machineHeader.machineId == machineId) {
            mc.machineHeader.stellerCompensation_mm = newVal;

            CuttingInstructionUtils::postProcessMachineCuts(
                mc,
                CuttingInstructionUtils::SortStrategy::BySizeDesc
                );
            break;
        }
    }

    if (view)
        view->renderCuttingInstructions(_machineCutsList);
}

void CuttingPresenter::cloneRequestDialog() {

    CloneRequestDialog dlg(view);
    if (dlg.exec() != QDialog::Accepted) return;

    auto rules = dlg.result();     // material‑wise szabályok
    QString tag = dlg.tag();       // fájlnév tag

    cloneRequest(rules, tag);

}

void CuttingPresenter::cloneRequest(const QVector<CloneMaterialRule>& rules, const QString& tag)
{
    QVector<Cutting::Plan::Request> all = CuttingPlanRequestRegistry::instance().readAll();
    QVector<Cutting::Plan::Request> cloned;

    for (const auto& req : all) {

        Cutting::Plan::Request r = req;
        r.requestId = QUuid::createUuid();

        // Megkeressük a request materialjához tartozó szabályt
        for (const auto& rule : rules) {
            if (rule.originalMaterialId == req.materialId) {

                // Anyagcsere csak akkor, ha a user választott újat
                if (!rule.newMaterialId.isNull())
                    r.materialId = rule.newMaterialId;

                // Hossz delta csak akkor, ha nem 0
                if (rule.delta != 0)
                    r.requiredLength = req.requiredLength + rule.delta;

                break;
            }
        }

        cloned.append(r);
    }

    // Új fájlnév generálása
    QString oldName = SettingsManager::instance().cuttingPlanFileName();
    QString base = QFileInfo(oldName).completeBaseName();
    int idx = base.indexOf('_', QString("cuttingplan_YYYYMMDD-HHMMSS").size());
    QString prefix = (idx == -1) ? base : base.left(idx);
    QString newName = prefix + "_" + tag + ".txt";

    // Mentés
    SettingsManager::instance().setCuttingPlanFileName(newName);
    CuttingPlanRequestRegistry::instance().setData(cloned);

    // GUI frissítés
    if (view) {
        QString full = FileNameHelper::instance().getCuttingPlanFilePath(newName);
        view->setInputFileLabel(newName, full);
        view->refresh_InputTableFromRegistry();
    }
}

void CuttingPresenter::ExportLeftoverIntakeForm_Pdf()
{
    int rowsPerPage = 15;

    QString fileName = SettingsManager::instance().cuttingPlanFileName();
    QFileInfo fi(fileName);
    QString baseName = fi.completeBaseName();

    if (baseName.isEmpty()) {
        zEvent("❌ Nincs Cutting Plan fájlnév — leftover PDF export nem lehetséges.");
        return;
    }

    QString dir = fi.absolutePath() + "/_reports";
    QDir().mkpath(dir);

    int start = SettingsManager::instance().peekManualLeftoverCounter();
    int end   = start + rowsPerPage - 1;

    QString path = QString("%1/leftoverintakeform_RSM-%2-%3.pdf")
                       .arg(dir)
                       .arg(start, 3, 10, QChar('0'))
                       .arg(end,   3, 10, QChar('0'));

    QPdfWriter writer(path);
    writer.setPageSize(QPageSize(QPageSize::A4));
    writer.setResolution(300);

    QPainter painter(&writer);
    if (!painter.isActive()) {
        zEvent("❌ Nem sikerült megnyitni a PDF fájlt.");
        return;
    }

    QRectF pageRect = writer.pageLayout().paintRectPixels(writer.resolution());
    painter.setFont(QFont("Noto Sans Mono", 11));

    CuttingInstructionUtils::formatLeftoverIntakeForm_Pdf(
        painter,
        writer,
        pageRect,
        rowsPerPage
        );

    painter.end();
    zEvent(QString("📄 Leftover Intake Form PDF exportálva: %1").arg(path));
}


PaintPlan CuttingPresenter::buildPaintPlan()
{
    PaintPlan plan;

    // 1) Minden request beolvasása
    const auto all = CuttingPlanRequestRegistry::instance().readAll();

    // 2) Csoportosítás tételszám szerint
    QHash<QString, QVector<Cutting::Plan::Request>> groups;
    for (const auto& req : all) {
        QString ref = req.externalReference.trimmed();
        if (ref.isEmpty())
            ref = "<NINCS_TETELSZAM>";
        groups[ref].append(req);
    }

    // 3) Minden tételszám-csoport feldolgozása
    for (auto it = groups.begin(); it != groups.end(); ++it)
    {
        const auto& list = it.value();

        // --- TÍPUSDETEKTÁLÁS ---
        NaphaloType type = NaphaloTypeDetector::detect(list);

        // --- A CSOPORT ÖSSZES REQUESTJÉNEK FELDOLGOZÁSA ---
        for (const auto& req : list)
        {
            QString color = req.color.trimmed();
            if (color.isEmpty())
                color = QStringLiteral("Nincs szín megadva");

            auto& colorGroup = plan.byColor[color];
            colorGroup.color = color;

            // Anyag lekérése
            const MaterialMaster* mat = MaterialRegistry::instance().findById(req.materialId);
            QString barcode = mat ? mat->barcode : "";

            // Szorzó (CL/SL → 2)
            int szorzo = 1;
            if (matchPrefix(barcode, "NP-CL") || matchPrefix(barcode,"NP-SL"))
                szorzo = 2;

            // ANYAG AGGREGÁLÁS
            auto& summary = colorGroup.materials[req.materialId];
            summary.materialId = req.materialId;
            summary.totalPieces += req.quantity * szorzo;
            summary.totalLength_mm += req.quantity * req.requiredLength * szorzo;
            summary.requestIds.append(req.requestId);

            // --- POFA / CSAVAR TÍPUS SZERINT ---
            if (matchPrefix(barcode, "NP-T")) {
                if (type == NaphaloType::Cipzaros) colorGroup.cipzarosPofa += 2;
                if (type == NaphaloType::Sines)    colorGroup.sinesPofa += 2;
                if (type == NaphaloType::Bowdenes) colorGroup.bowdenesPofa += 2;
            }

            if (matchPrefix(barcode, "NP-TF")) {
                colorGroup.csavar +=2;
                // if (type == NaphaloType::Cipzaros) colorGroup.cipzarosCsavar += 2;
                // if (type == NaphaloType::Sines)    colorGroup.sinesCsavar += 2;
                // if (type == NaphaloType::Bowdenes) colorGroup.bowdenesCsavar += 2;
            }
        }
    }

    return plan;
}

void CuttingPresenter::Paint()
{
    zInfo("=== FESTÉSI TERV (TXT) ===");

    PaintPlan plan = buildPaintPlan();

    // Színek ábécé sorrendben
    QStringList colors = plan.byColor.keys();
    colors.sort(Qt::CaseInsensitive);

    for (const QString& color : colors)
    {
        const auto& colorGroup = plan.byColor[color];

        zInfo(QString(">> SZÍN: %1").arg(color));
        zInfo("----------------------------------------");

        // Anyagok ábécé sorrendben
        QList<QUuid> materials = colorGroup.materials.keys();
        std::sort(materials.begin(), materials.end(),
                  [](const QUuid& a, const QUuid& b){
                      return a.toString() < b.toString();
                  });

        for (const QUuid& matId : materials)
        {
            const auto& s = colorGroup.materials[matId];

            const MaterialMaster* mat = MaterialRegistry::instance().findById(matId);
            QString matName = mat ? mat->toDisplay() : "???";

            zInfo(QString("   Anyag: %1").arg(matName));

            QString barcode = mat?mat->barcode:"???";
            QString postfix = profilePostfixFor(barcode);

            if (!postfix.isEmpty()) {
                zInfo(QString("      Teljes hossz: %1 mm (%2 m × %3)")
                          .arg(s.totalLength_mm)
                          .arg(s.totalLength_mm / 1000.0, 0, 'f', 2)
                          .arg(postfix));
            } else {
                zInfo(QString("      Teljes hossz: %1 mm (%2 m)")
                          .arg(s.totalLength_mm)
                          .arg(s.totalLength_mm / 1000.0, 0, 'f', 2));
            }


            // Request ID lista
            QStringList tetelszamok;
            for (const auto& id : s.requestIds) {
                auto *a = CuttingPlanRequestRegistry::instance().findById(id);
                QString b = a ? a->externalReference : "?";
                QString c = a ? a->dueDate.toString("yyyy-MM-dd") : "?";
                tetelszamok << QString("%1 (%2)").arg(b, c);
            }

            zInfo(QString("      Tételszámok: %1").arg(tetelszamok.join(", ")));
            zInfo("");
        }

        // --- TÍPUSONKÉNTI POFÁK ---
        zInfo("   POFÁK:");
        zInfo(QString("      Cipzáros: %1").arg(colorGroup.cipzarosPofa));
        zInfo(QString("      Sines:    %1").arg(colorGroup.sinesPofa));
        zInfo(QString("      Bowdenes: %1").arg(colorGroup.bowdenesPofa));

        QString postfix1 = profilePostfixFor("NP-POF");
        if (!postfix1.isEmpty()) {
            zInfo(QString("      Összesen: %1 db, %2").arg(colorGroup.sumPofa()).arg(postfix1));
        } else{
            zInfo(QString("      Összesen: %1 db").arg(colorGroup.sumPofa()));
        }
        // --- TÍPUSONKÉNTI CSAVAROK ---
        zInfo("   CSAVAROK:");
        QString postfix2 = profilePostfixFor("NP-CSAV");
        if (!postfix2.isEmpty()) {
            zInfo(QString("      Összesen: %1 db, %2").arg(colorGroup.csavar).arg(postfix2));
        } else{
            zInfo(QString("      Összesen: %1 db").arg(colorGroup.csavar));
        }
        // zInfo(QString("      Cipzáros: %1").arg(colorGroup.cipzarosCsavar));
        // zInfo(QString("      Sines:    %1").arg(colorGroup.sinesCsavar));
        // zInfo(QString("      Bowdenes: %1").arg(colorGroup.bowdenesCsavar));

        zInfo("========================================");
    }

    zInfo("=== FESTÉSI TERV VÉGE ===");
}


void CuttingPresenter::AuditRequestsByExternalRef_old()
{
    zInfo("=== AUDIT: TÉTELSZÁM KONSZISZTENCIA VIZSGÁLAT - RÉGI TÍPUSÚ ===");


    const auto all = CuttingPlanRequestRegistry::instance().readAll();

    QHash<QString, CountPerType> summary;

    // tételszám → request lista
    QHash<QString, QVector<Cutting::Plan::Request>> groups;

    for (const auto& req : all) {
        QString ref = req.externalReference.trimmed();
        if (ref.isEmpty())
            ref = "<NINCS_TETELSZAM>";
        groups[ref].append(req);
    }

    // bejárás tételszámonként
    for (auto it = groups.begin(); it != groups.end(); ++it)
    {
        bool hasError = false;

        const QString& ref = it.key();
        const auto& list = it.value();

        zInfo(QString(">> Tételszám: %1").arg(ref));
        // Megrendelő neve
        QString customer = "<ismeretlen>";
        if (!list.isEmpty()) {
            customer = list.first().ownerName.trimmed();
            if (customer.isEmpty())
                customer = "<ismeretlen>";
        }



        zInfo(QString("   Megrendelő: %1").arg(customer));
        // Határidő
        QDate due = QDate(); // invalid default
        if (!list.isEmpty()) {
            due = list.first().dueDate;
        }

        QString dueStr = due.isValid() ? due.toString("yyyy-MM-dd") : "<nincs>";
        zInfo(QString("   Határidő: %1").arg(dueStr));

        // --- OWNER NAME KONZISZTENCIA ELLENŐRZÉS ---
        QString expectedOwner = customer; // amit az előbb kiírtunk

        for (const auto& req : list) {
            QString o = req.ownerName.trimmed();
            if (o.isEmpty()) o = "<ismeretlen>";

            if (o != expectedOwner) {
                zInfo(QString("   ❌ HIBA: Eltérő megrendelő név található! (%1 vs %2)")
                          .arg(o)
                          .arg(expectedOwner));
            }
        }

        // --- HATÁRIDŐ KONZISZTENCIA ELLENŐRZÉS ---
        QDate expectedDue = due;

        for (const auto& req : list) {
            if (req.dueDate != expectedDue) {
                zInfo(QString("   ❌ HIBA: Eltérő határidő található! (%1 vs %2)")
                          .arg(req.dueDate.toString("yyyy-MM-dd"))
                          .arg(expectedDue.toString("yyyy-MM-dd")));
            }
        }

        zInfo("----------------------------------------");

        // anyag → darabszám
        QHash<QString, int> materialCount;
        QHash<QString, int> materialLength;

        int tokCount = 0;
        int tokFedCount = 0;
        int labCount = 0;
        int labbetetCount = 0;
        int czCount = 0;
        int szCount = 0;

        for (const auto& req : list)
        {
            const MaterialMaster* mat = MaterialRegistry::instance().findById(req.materialId);
            QString barcode = mat ? mat->barcode : "???";

            int qty = req.quantity;
            int len = req.requiredLength;

            // CIPZÁROS LÁB
            if (matchPrefix(barcode,"NP-CL"))
                labCount += qty * 2;

            // CIPZÁROS LÁBBETÉT
            if (matchPrefix(barcode,"NP-CLB"))
                labbetetCount += qty * 2;

            if (matchPrefix(barcode, "NP-CLBR"))
                labbetetCount += qty * 2;

            // SINES LÁB
            if (matchPrefix(barcode , "NP-SL" ))
                labCount += qty * 2;

            // TOK
            if (matchPrefix(barcode , "NP-T"))
                tokCount += qty;

            // TOKFEDÉL
            if (matchPrefix(barcode , "NP-TF"))
                tokFedCount += qty;

            // CIPZÁROS ZÁRÓ
            if (matchPrefix(barcode , "NP-CZ"))
                czCount += qty;

            // SINES ZÁRÓ
            if (matchPrefix(barcode, "NP-SZ" ))
                szCount += qty;

            materialCount[barcode] += qty;
            materialLength[barcode] += qty * len;
        }

        // anyaglista kiírása
        for (auto it2 = materialCount.begin(); it2 != materialCount.end(); ++it2)
        {
            QString barcode = it2.key();
            int qty = it2.value();
            int totalLen = materialLength[barcode];

            zInfo(QString("   Anyag: %1 | db: %2 | hossz: %3 mm")
                      .arg(barcode)
                      .arg(qty)
                      .arg(totalLen));
        }

        // összegzés
        zInfo(QString("   Tok darabszám: %1").arg(tokCount));
        zInfo(QString("   Tokfedél darabszám: %1").arg(tokFedCount));
        zInfo(QString("   Láb darabszám: %1").arg(labCount));
        zInfo(QString("   Lábbetét darabszám: %1").arg(labbetetCount));
        zInfo(QString("   Cipzáros záró: %1").arg(czCount));
        zInfo(QString("   Sines záró: %1").arg(szCount));

        // --- TÍPUS MEGHATÁROZÁSA ---
        bool isCipzaros = (czCount > 0);
        bool isSines = (szCount > 0 && labCount > 0);
        bool isBowdenes = (szCount > 0 && labCount == 0);

        if (isCipzaros) zInfo("   Típus: CIPZÁROS");
        if (isSines)    zInfo("   Típus: SINES");
        if (isBowdenes) zInfo("   Típus: BOWDENES");
        if (!isCipzaros && !isSines && !isBowdenes){
            zInfo("   ⚠️ Típus: ISMERETLEN / HIÁNYOS ADAT");
            hasError = true;
        }

        // --- TOK ↔ TOKFEDÉL ---
        if (tokCount != tokFedCount){
            zInfo(QString("   ⚠️ ELTÉRÉS: Tok (%1) és tokfedél (%2) darabszáma nem egyezik!")
                      .arg(tokCount).arg(tokFedCount));
            hasError = true;
        }

        // --- CIPZÁROS BOM ---
        if (isCipzaros)
        {
            int expectedLab = tokCount * 2;
            int expectedLabBetet = tokCount * 2;
            int expectedCZ = tokCount;

            if (labCount != expectedLab){
                zInfo(QString("   ⚠️ CIPZÁROS: Láb (%1) != elvárt (%2)")
                          .arg(labCount).arg(expectedLab));
                hasError = true;
            }


            if (labbetetCount != expectedLabBetet){
                zInfo(QString("   ⚠️ CIPZÁROS: Lábbetét (%1) != elvárt (%2)")
                          .arg(labbetetCount).arg(expectedLabBetet));
                hasError = true;
            }

            if (czCount != expectedCZ){
                zInfo(QString("   ⚠️ CIPZÁROS: Záró (%1) != elvárt (%2)")
                          .arg(czCount).arg(expectedCZ));
                hasError = true;
            }
        }

        // --- SINES BOM ---
        if (isSines)
        {
            int expectedLab = tokCount * 2;
            int expectedLabBetet = tokCount * 2;
            int expectedSZ = tokCount;

            if (labCount != expectedLab){
                zInfo(QString("   ⚠️ SINES: Láb (%1) != elvárt (%2)")
                          .arg(labCount).arg(expectedLab));
                hasError = true;
            }

            if (labbetetCount != expectedLabBetet){
                zInfo(QString("   ⚠️ SINES: Lábbetét (%1) != elvárt (%2)")
                          .arg(labbetetCount).arg(expectedLabBetet));
                hasError = true;
            }

            if (szCount != expectedSZ){
                zInfo(QString("   ⚠️ SINES: Záró (%1) != elvárt (%2)")
                          .arg(szCount).arg(expectedSZ));
                hasError = true;
            }
        }

        // --- BOWDENES BOM ---
        if (isBowdenes)
        {
            if (labCount > 0){
                zInfo("   ⚠️ BOWDENES: Láb nem lehet, de van!");
                hasError = true;
            }

            if (labbetetCount > 0){
                zInfo("   ⚠️ BOWDENES: Lábbetét nem lehet, de van!");
                hasError = true;
            }

            if (szCount != tokCount){
                zInfo(QString("   ⚠️ BOWDENES: Sines záró (%1) != tok (%2)")
                          .arg(szCount).arg(tokCount));
                hasError = true;
            }
        }


        auto& s = summary[customer];

        s.total++;

        if (isCipzaros) s.cipzaros++;
        if (isSines)    s.sines++;
        if (isBowdenes) s.bowdenes++;

        if (hasError) {
            s.bad++;
            s.badRefs << ref;   // <-- hibás tételszám hozzáadása
        } else {
            s.good++;
        }



    }


    zInfo("=== ÖSSZESÍTÉS MEGRENDELŐNKÉNT ===");

    for (auto it = summary.begin(); it != summary.end(); ++it) {
        const QString& customer = it.key();
        const CountPerType& s = it.value();

        QStringList parts;

        if (s.cipzaros > 0)
            parts << QString("CIPZÁROS=%1").arg(s.cipzaros);

        if (s.sines > 0)
            parts << QString("SINES=%1").arg(s.sines);

        if (s.bowdenes > 0)
            parts << QString("BOWDENES=%1").arg(s.bowdenes);

        QString typeSummary = parts.isEmpty()
                                  ? "nincs napháló"
                                  : parts.join(", ");

        QString qualitySummary =
            QString("jó=%1, hibás=%2").arg(s.good).arg(s.bad);

        QString icon;
        if (s.total > 0 && s.bad == 0)
            icon = "✅";        // minden jó
        else if (s.bad > 0)
            icon = "❌";        // van hiba
        else
            icon = "•";         // nincs napháló

        QString badList;
        if (!s.badRefs.isEmpty())
            badList = QString(" (%1)").arg(s.badRefs.join(", "));  // pl. (1234, 1235, 1236)

        zInfo(QString("   %1 %2: %3 | %4%5")
                  .arg(icon)
                  .arg(customer)
                  .arg(typeSummary)
                  .arg(qualitySummary)
                  .arg(badList));


    }

    zInfo("=== ÖSSZESÍTÉS VÉGE ===");

    zInfo("=== AUDIT VÉGE - RÉGI TÍPUSÚ ===");
}

void CuttingPresenter::Audit()
{
    AuditRequestsByExternalRef_old();

    AuditRequestsByExternalRef();
}

void CuttingPresenter::AuditRequestsByExternalRef()
{
    zInfo("=== AUDIT: TÉTELSZÁM KONSZISZTENCIA VIZSGÁLAT ===");

    const auto all = CuttingPlanRequestRegistry::instance().readAll();
    auto result = NaphaloAuditService::run(all);

    // 🔹 Üzenetek kiírása
    for (const auto& m : result.messages) {
        QString line = QString(">> [%1] %2")
        .arg(m.ref)
            .arg(m.text);
        zInfo(line);
    }

    // 🔹 Summary kiírása
    zInfo("=== ÖSSZESÍTÉS MEGRENDELŐNKÉNT ===");

    for (auto it = result.summary.begin(); it != result.summary.end(); ++it) {
        const QString& customer = it.key();
        const CountPerType& s = it.value();

        QStringList parts;
        if (s.cipzaros > 0) parts << QString("CIPZÁROS=%1").arg(s.cipzaros);
        if (s.sines > 0)    parts << QString("SINES=%1").arg(s.sines);
        if (s.bowdenes > 0) parts << QString("BOWDENES=%1").arg(s.bowdenes);

        QString typeSummary = parts.isEmpty()
                                  ? "nincs napháló"
                                  : parts.join(", ");

        QString qualitySummary =
            QString("jó=%1, hibás=%2").arg(s.good).arg(s.bad);

        QString icon;
        if (s.total > 0 && s.bad == 0) icon = "✅";
        else if (s.bad > 0)            icon = "❌";
        else                           icon = "•";

        QString badList;
        if (!s.badRefs.isEmpty())
            badList = QString(" (%1)").arg(s.badRefs.join(", "));

        QString line = QString("   %1 %2: %3 | %4%5")
                           .arg(icon)
                           .arg(customer)
                           .arg(typeSummary)
                           .arg(qualitySummary)
                           .arg(badList);

        zInfo(line);
    }

    zInfo("=== AUDIT VÉGE ===");
}



/*relocation*/


