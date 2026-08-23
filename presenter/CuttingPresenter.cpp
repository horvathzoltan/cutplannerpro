#include <QDir>
#include <QFileInfo>
#include <QPdfWriter>
#include <ui_clonerequestdialog.h>

#include "CuttingPresenter.h"
#include "../view/MainWindow.h"

#include "../common/logger.h"
#include "common/eventlogger.h"

#include "../model/storageaudit/storageauditentry.h"
#include "../service/cutting/result/archivedwasteutils.h"
#include "../model/registries/cuttingplanrequestregistry.h"
#include "leftover/registry/leftoverstockregistry.h"
#include "service/cutting/plan/cuttingplan_validationservice.h"
#include "service/snapshot/inventorysnapshot_validator.h"
#include "stock/registry/stockregistry.h"
#include "../service/cutting/optimizer/exporter.h"
#include "../common/filenamehelper.h"
#include "settings/settingsmanager.h"
#include "../model/repositories/cuttingrequestrepository.h"
#include "materials/registry/material_registry.h"
#include "../service/cutting/optimizer/optimizationlogger.h"
#include "../service/cutting/optimizer/optimizationrunner.h"
#include "../service/cutting/optimizer/optimizationviewupdater.h"
#include "../service/snapshot/requestsnapshotbuilder.h"
//#include "materials/utils/material_group_utils.h"
#include "service/cutting/instruction/cuttinginstructionutils.h"
//#include "service/cutting/summary/cutplansummary.h"
//#include "service/cutting/summary/cutplansummarybuilder.h"
#include "cutting/export/cutinstructionservice.h"
#include "service/snapshot/inventorysnapshotbuilder.h"
#include "model/cutting/optimizer/bundle_overcuttingdetector.h"
//#include <model/registries/cuttingmachineregistry.h>
//#include <model/repositories/cuttingrequestrepository.h>
//#include <model/cutting/plan/audit/naphalo_audit_types.h>
//#include <model/cutting/plan/audit/product_bom_audit_service.h>

CuttingPresenter::CuttingPresenter(MainWindow* view, QObject *parent)
    : QObject(parent), _view(view) {}

// ez új cutting plant csinál és új néven kezdi perzisztálni
void CuttingPresenter::createNew_CuttingPlanRequests() {
    QString newFileName = FileNameHelper::instance().getNew_CuttingPlanFileName();
    QString newFilePath = FileNameHelper::instance().getCuttingPlanFilePath(newFileName);

    // 🔄 Állapot frissítése
    SettingsManager::instance().setCuttingPlanFileName(newFileName);

    removeAll_CuttingPlanRequests();

    // 🧹 GUI frissítés - beírjuk az új file nevet a labelbe
    if (_view) {
        _view->setInputFileLabel(newFileName, newFilePath);
    }
}

void CuttingPresenter::removeAll_CuttingPlanRequests() {
    // 🧹 Táblázat törlése a GUI-ban
    if (_view) {
        _view->clear_InputTable();
    }
    // 🗃️ Registry kiürítése
    CuttingPlanRequestRegistry::instance().clearAll();
}

/*input*/
void CuttingPresenter::add_CuttingPlanRequest(const Cutting::Plan::Request& req) {
    CuttingPlanRequestRegistry::instance().registerRequest(req);
    if(_view){
         _view->addRow_InputTable(req);
    }
 }

void CuttingPresenter::applyPatch(Cutting::Plan::Request& target,
                                  const Cutting::Plan::Request& updated,
                                  const CuttingRequestPatch& patch)
{
    if(patch.updateWidthHeight){
        target.fullWidth_mm = updated.fullWidth_mm;
        target.fullHeight_mm = updated.fullHeight_mm;
    }

    if (patch.updateLength)
        target.requiredLength = updated.requiredLength;

    if (patch.updateMaterial)
        target.materialId = updated.materialId;

    if (patch.updateLeftRight) {
        target.leftCount  = updated.leftCount;
        target.rightCount = updated.rightCount;
        target.quantity   = updated.quantity;
    }

    if (patch.updateOwner)
        target.ownerName = updated.ownerName;

    if (patch.updateDueDate)
        target.dueDate = updated.dueDate;

    if (patch.updateProductType)
        target.productTypeId = updated.productTypeId;

    if (patch.updateProductSubtype)
        target.productSubtypeId = updated.productSubtypeId;

    if (patch.updateColor){
        target.requiredColor = updated.requiredColor;
        target.surface = updated.surface;
    }

    if(patch.updateAttributes)
        target.attributes = updated.attributes;
}

void CuttingPresenter::update_CuttingPlanRequest(const Cutting::Plan::Request& r) {
    auto opt = CuttingPlanRequestRegistry::instance().findById(r.requestId);
    if (!opt) {
        qWarning() << "❌ Sikertelen frissítés: nincs ilyen requestId:" << r.requestId;
        return;
    }

    Cutting::Plan::Request target = *opt;

    // ezeket végigpatcheli sz összes fej szintű adaton
    CuttingRequestPatch patch;
    patch.updateOwner       = true;
    patch.updateLength    = true;
    patch.updateWidthHeight    = true;
    patch.updateMaterial  = true;
    patch.updateColor = true;
    patch.updateAttributes = true;
    patch.updateLeftRight = true;
    patch.updateProductSubtype = true;

    applyPatch(target, r, patch);

    bool ok = CuttingPlanRequestRegistry::instance().updateRequest(target);

    if (ok) {
        if (_view) {
            _view->updateRow_InputTable(target);
        }
    } else {
        qWarning() << "❌ Sikertelen frissítés: nincs ilyen requestId:" << r.requestId;
        return;
    }
}

void CuttingPresenter::update_AllRequestsWithSameReference(
    const Cutting::Plan::Request& updated)
{
    CuttingRequestPatch patch;
    patch.updateOwner         = true;
    patch.updateDueDate       = true;
    patch.updateProductType   = true;
    patch.updateProductSubtype= true;
    patch.updateColor         = true;
    patch.updateLeftRight     = true;

    auto all = CuttingPlanRequestRegistry::instance().readAll();

    for (auto& req : all) {
        // if (req.requestId == updated.requestId)
        //     continue;
        if (req.externalReference == updated.externalReference) {
            applyPatch(req, updated, patch);

            bool ok = CuttingPlanRequestRegistry::instance().updateRequest(req);
            if (ok && _view) {
                _view->updateRow_InputTable(req);
            }
        }
    }
}

void CuttingPresenter::remove_CuttingPlanRequest(const QUuid& requestId) {
    CuttingPlanRequestRegistry::instance().removeRequest(requestId);  // ✅ Globális törlés
    if(_view){
        _view->removeRow_InputTable(requestId);
    }
}

/*stock*/
void CuttingPresenter::add_StockEntry(const StockEntry& entry) {
    StockRegistry::instance().registerEntry(entry);
    if(_view){
        _view->addRow_StockTable(entry);
    }
    auto sp = _view->storageAuditPresenter();
    auto m = sp->auditStateManager();

    m->setOutdated(AuditStateManager::AuditOutdatedReason::StockChanged);
}

void CuttingPresenter::remove_StockEntry(const QUuid& stockId) {
    StockRegistry::instance().removeEntry(stockId);   // ✅ Globális törlés
    if (_view) {
        _view->removeRow_StockTable(stockId); // ha a készlet változik
    }

    auto sp = _view->storageAuditPresenter();
    auto m = sp->auditStateManager();

    m->notifyStockRemoved(stockId);
}

void CuttingPresenter::update_StockEntry(const StockEntry& updated) {
    bool ok = StockRegistry::instance().updateEntry(updated); // 🔁 adatbázis update

    if (ok){
        if(_view){
            _view->updateRow_StockTable(updated);
        }
        auto sp = _view->storageAuditPresenter();
        auto m = sp->auditStateManager();

        m->notifyStockChanged(updated);
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
    if(_view){
        _view->addRow_LeftoversTable(req);
    }
    auto sp = _view->storageAuditPresenter();
    auto m = sp->auditStateManager();

    m->setOutdated(AuditStateManager::AuditOutdatedReason::LeftoverChanged);
}

bool CuttingPresenter::remove_LeftoverStockEntry(const QUuid& entryId) {
    bool ok = LeftoverStockRegistry::instance().removeEntry(entryId);

    if(!ok){
        qWarning() << "❌ Sikertelen törlés: nincs ilyen entryId:" << entryId;
        return false;
    }

    if(_view){
        _view->removeRow_LeftoversTable(entryId);
    }
    return true;
}

void CuttingPresenter::update_LeftoverStockEntry(const LeftoverStockEntry& updated) {
    bool ok = LeftoverStockRegistry::instance().updateEntry(updated); // 🔁 Frissítés Registry-ben

    if (ok) {
        if (_view) {
            _view->updateRow_LeftoversTable(updated);
        }
    }
    else
    {
        qWarning() << "❌ Sikertelen frissítés: nincs ilyen entryId:" << updated.entryId;
        return;
    }
}


void CuttingPresenter::setCuttingRequests(const QVector<Cutting::Plan::Request>& list) {
    _optimizerModel.setCuttingRequests(list);
}

// void CuttingPresenter::setStockInventory(const QVector<StockEntry>& list) {
//     model.setStockInventory(list);
// }

// void CuttingPresenter::setReusableInventory(const QVector<LeftoverStockEntry>& list) {
//     model.setReusableInventory(list);
// }

// void CuttingPresenter::setKerf(int kerf) {
//     _optimizerModel.setKerf(kerf);
// }

const QVector<Cutting::Plan::CutPlan>& CuttingPresenter::getPlansRef() const
{
    return _optimizerModel.getResult_PlansRef();
}

QVector<Cutting::Result::ResultModel> CuttingPresenter::getLeftoverResults()
{
    return _optimizerModel.getResults_Leftovers();
}



//     // 🖥️ Végül frissítjük az Audit táblát a nézetben
//     _view->update_StorageAuditTable(lastAuditRows);
// }
void CuttingPresenter::runOptimization(Cutting::Optimizer::TargetHeuristic heuristic) {
    if (!isModelSynced) {
        zWarning(L("⚠️ Modell nincs szinkronizálva optimalizáció előtt!"));
        return;
    }

        // 🔧 Hulló készlet használatának beállítása
    _optimizerModel.setUseReusableLeftovers(
        _view->isChkUseLeftoversChecked()
        );

    zInfo("▶️ OptimizationRunner::run started");
    // 1️⃣ Optimalizáció futtatása
    OptimizationRunner::run(_optimizerModel, heuristic);
    zInfo("⏹️ OptimizationRunner::run stopped");
    // 5️⃣ Logolás
    OptimizationLogger::logPlans(_optimizerModel.getResult_PlansRef());

    // 2️⃣ Nézet frissítése
    //if (_view) {
        //OptimizationViewUpdater::update(_view, _optimizerModel);
        refreshAllViews(Refresh::Flags::SnapshotOnly);
        //_view->switchToCuttingPlanTab();   // ⬅️ EZT ADJUK HOZZÁ
    //}

    // 3️⃣ Export (opcionális)
    //OptimizationExporter::exportPlans(model.getResult_PlansRef());
    saveOptimizationSnapshot();

    // 4️⃣ Audit sorok előállítása
    auto sp = _view->storageAuditPresenter();
    auto am = sp->auditStateManager();

    am->setOutdated(AuditStateManager::AuditOutdatedReason::OptimizeRun);
    //lastAuditRows = OptimizationAuditBuilder::build(model);

    isModelSynced = false;

    auto over = BundleOverCuttingDetector::detect(_optimizerModel);

    // 🔍 bundling előtti állapot logolása
    BundleOverCuttingDetector::logRequests("🔍 BEFORE postProcess", over.newRequests);

    // ⭐ bundle-aware utófeldolgozás
    over.newRequests_2 = BundleOverCuttingDetector::postProcessBundleOvercuts(over);

    // 🔍 bundling utáni állapot logolása
    BundleOverCuttingDetector::logRequests("🔍 AFTER postProcess", over.newRequests_2);


    if (over.hasOvercuts) {
        zWarning("⚠️ Bundle túlvágás detektálva — pótló optimalizálás indul");

        Cutting::Optimizer::OptimizerModel extra;
        extra.setCuttingRequests(over.newRequests_2);

        auto lengthsPerMaterial =
            RequestSnapshotBuilder::getLengthsPerMaterial(over.newRequests_2);

        auto expanded =
            RequestSnapshotBuilder::expandLengthsWithGroupMembers(lengthsPerMaterial);

        QMap<QUuid, int> strandsPerMaterial =
            InventorySnapshotBuilder::greedyStrandPacking(expanded);

        InventorySnapshot extraNeeded =
            InventorySnapshotBuilder::build2(strandsPerMaterial);

        // 🔁 A fő optimizer által már megfogyasztott snapshot másolata
        InventorySnapshot finalSnap = _optimizerModel.inventorySnapshot();

        // hozzáadjuk az extra szálakat
        for (const auto& stock : extraNeeded.profileInventory) {
            finalSnap.profileInventory.append(stock);
        }

        extra.setInventorySnapshot(finalSnap);

        extra.optimize(Cutting::Optimizer::TargetHeuristic::ByCount);

        // 🔗 Az extra futás terveit hozzácsapjuk az eredetihez
        auto extraPlans = extra.getResult_PlansRef();
        auto& mainPlans =
            const_cast<QVector<Cutting::Plan::CutPlan>&>(_optimizerModel.getResult_PlansRef());
        mainPlans.append(extraPlans);
    }

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
    QVector<Cutting::Plan::Request> requests = RequestSnapshotBuilder::build();

    auto result1 = CuttingPlanValidationService::validate(requests);
    if (_view)
        _view->ShowWarningDialog(result1);

    if(result1.hasError()){
        isModelSynced = false;
        return;
    }

    // 4️⃣ Request lista kiírása (debug)
    zInfo("📦 syncModelWithRegistries requests:");
    for (const auto &r : requests) {
        const MaterialMaster *mat =
            MaterialRegistry::instance().findById(r.materialId);
        zInfo(QString("   •  %1. %2 %3, %4 mm, %5 db ")
                  .arg(r.externalReference)
                  .arg(r.ownerName)
                  .arg(mat ? mat->toDisplay() : "(?)")
                  .arg(r.requiredLength)
                  .arg(r.quantity));
    }

    // 5️⃣ Igény → szálak → inventory
    auto lengthsPerMaterial = RequestSnapshotBuilder::getLengthsPerMaterial(requests);
    auto expandedlengths = RequestSnapshotBuilder::expandLengthsWithGroupMembers(lengthsPerMaterial);
    QMap<QUuid, int> strandsPerMaterial =
        InventorySnapshotBuilder::greedyStrandPacking(expandedlengths);
    InventorySnapshot inventory =
        InventorySnapshotBuilder::build2(strandsPerMaterial);

    auto result2 = InventorySnapshotValidator::validate(inventory, strandsPerMaterial);
    if (_view)
        _view->ShowWarningDialog(result2);

    if (result2.hasError()) {
        isModelSynced = false;
        return;
    }

    // 8️⃣ Modell betöltése
    _optimizerModel.setCuttingRequests(requests);
    _optimizerModel.setInventorySnapshot(inventory);
    isModelSynced = true;
}

bool CuttingPresenter::loadCuttingPlanFromFile(const QString& path) {
    bool loaded = CuttingRequestRepository::loadFromFile(CuttingPlanRequestRegistry::instance(), path);
    return loaded;
}

void CuttingPresenter::GenerateCutInstructions(SortMode mode,
                                               const QVector<QString>& prioRefs)
{

    const QVector<Cutting::Plan::CutPlan> &cutPlans = _optimizerModel.getResult_PlansRef();

    if (cutPlans.isEmpty()) {
        ValidationResult r;
        r.errors << "Nincs optimalizációs eredmény.\n"
                    << "Előbb futtasd az Optimize műveletet.";
        if (_view)
            _view->ShowWarningDialog(r);
        return;
    }

    _machineCutsList.clear();

    //auto& cutPlans = _optimizerModel.getResult_PlansRef();
    auto leftovers = _optimizerModel.getResults_Leftovers();

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

        // 🔥 leftover beemelése — MOST már biztonságos
        QString rodKey = plan.source == Cutting::Plan::Source::Reusable
                             ? plan.sourceBarcode
                             : plan.rodId;

        it->leftoverInfo.leftover_mm[rodKey] = plan._segments.waste_mm();
        it->leftoverInfo.leftoverBarcode[rodKey] = plan._segments.leftoverBarcode();
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
//            ci.subtype = pwm.subtype;
            ci.productTypeId = pwm.productTypeId;
            ci.productSubtypeId = pwm.productSubtypeId;
            ci.attributes = pwm.attributes;
            // PATCH 13/B — toldás szerepkör átvezetése a gépi utasításba
            ci.toldasRole = pwm.info.toldasRole;
            ci.keepWhole = pwm.info.keepWhole;

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

    CutInstructionService::sort(&_machineCutsList, mode, prioRefs);

    // 🟦 MachineReport feltöltése (actualPieces)
    for (auto& mc : _machineCutsList) {
        _optimizerModel.setMachineActualPieces(
            mc.machineHeader.machineId,
            mc.cutInstructions.size()
            );
    }

    // ⚠️ GÉPENKÉNTI EXPECTED vs ACTUAL RIPORT
    for (const auto& mc : _machineCutsList) {
        const auto& rep = _optimizerModel.getMachineReport()[mc.machineHeader.machineId];

        if (rep.actualPieces < rep.expectedPieces) {
            zEvent(QString(
                       "⚠️ A %1 géphez %2 darab tartozna, de csak %3 vágható."
                       )
                       .arg(rep.machineName)
                       .arg(rep.expectedPieces)
                       .arg(rep.actualPieces));
        }
    }

    // UI frissítés
    if (_view){
        _view->renderCuttingInstructions(_machineCutsList);
        _view->switchToInstructionsPlanTab();   // ⬅️ EZT ADJUK HOZZÁ
    }
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

    if (_view)
        _view->renderCuttingInstructions(_machineCutsList);
}

void CuttingPresenter::cloneRequestDialog() {

    CloneRequestDialog dlg(_view);
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
    if (_view) {
        QString full = FileNameHelper::instance().getCuttingPlanFilePath(newName);
        _view->setInputFileLabel(newName, full);
        _view->refresh_InputTable();
    }
}

void CuttingPresenter::saveOptimizationSnapshot()
{
    auto& fn = FileNameHelper::instance();

    // 🔹 A vágási terv fájlneve mindig a Settings-ben van
    QString planFileName = SettingsManager::instance().cuttingPlanFileName();
    if (planFileName.isEmpty()) {
        zWarning("❗ Nincs cutting plan konfigurálva, snapshot nem menthető.");
        return;
    }

    // 🔹 A snapshot név a cutting plan névből + timestampből épül
    QString snapshotName = fn.getNew_OptimizationSnapshotFileName(planFileName);
    QString snapshotPath = fn.getOptimizationSnapshotFilePath(snapshotName);

    QDir().mkpath(fn.getOptimizationSnapshotFolder());

    _optimizerModel.saveSnapshot(snapshotPath);

    zInfo("🟢 Új optimalizációs snapshot készült: " + snapshotPath);
}


void CuttingPresenter::loadLatestSnapshotForCurrentPlan()
{
    auto& fnh = FileNameHelper::instance();

    QString planFileName = SettingsManager::instance().cuttingPlanFileName();
    if (planFileName.isEmpty())
        return;

    QString latestSnapshot = fnh.findLatestSnapshotForCuttingPlan(planFileName);
    if (latestSnapshot.isEmpty())
        return;

    zInfo("📦 Snapshot megtalálva: " + latestSnapshot);

    if (_optimizerModel.loadSnapshot(latestSnapshot)) {
        zInfo("🟢 Snapshot betöltve.");
        refreshAllViews(Refresh::Flags::SnapshotOnly);
    }/*else{
        refreshAllViews(Refresh::Flags::RequestOnly);
    }*/
}

void CuttingPresenter::refreshAllViews(Refresh::Flags flags)
{
    if (!_view) return;

    if (hasFlag(flags, Refresh::Flags::InputTable))
        _view->refresh_InputTable();

    if (hasFlag(flags, Refresh::Flags::StockTable))
        _view->refresh_StockTable();

    if (hasFlag(flags, Refresh::Flags::LeftoversTable))
        _view->refresh_LeftoversTable();

    if (hasFlag(flags, Refresh::Flags::ResultsTable)) {
        auto& plans = _optimizerModel.getResult_PlansRef();
        _view->update_ResultsTable(plans);
    }

    if (hasFlag(flags, Refresh::Flags::SwitchToTab))
        _view->switchToCuttingPlanTab();
}







QHash<QUuid, QVector<QUuid>> CuttingPresenter::collectUsedLeftoversFromPlans()
{
    const auto& cutPlans = _optimizerModel.getResult_PlansRef();

    QHash<QUuid, QSet<QUuid>> tmp;   // duplikációk kiszűrése gépenként

    for (const auto& plan : cutPlans) {

        // Csak reusable hulló érdekes
        if (plan.source == Cutting::Plan::Source::Reusable &&
            !plan.sourceBarcode.isEmpty())
        {
            auto entryOpt =
                LeftoverStockRegistry::instance().findByBarcode(plan.sourceBarcode);

            if (entryOpt) {
                QUuid machineId = plan.machineId;
                tmp[machineId].insert(entryOpt->entryId);
            }
        }
    }

    // QSet → QVector konverzió
    QHash<QUuid, QVector<QUuid>> result;
    for (auto it = tmp.begin(); it != tmp.end(); ++it) {
        result[it.key()] = it.value().values().toVector();
    }

    return result;
}

bool CuttingPresenter::isOptimizationAuditRequired(const QVector<QUuid>& usedIds)
{
    int hours = SettingsManager::instance().optimizationLeftoverAuditHours();
    QDateTime now = QDateTime::currentDateTime();

    for (const auto& id : usedIds) {

        auto entryOpt = LeftoverStockRegistry::instance().findById(id);
        if (!entryOpt)
            return true; // eltűnt → audit kell

        const auto& e = *entryOpt;

        bool tooOld = e.lastSeenAt < now.addSecs(-hours * 3600);
        bool lost   = e.notFoundCount > 0;

        if (tooOld || lost)
            return true;
    }

    return false; // minden friss → nem kell audit
}

QHash<QUuid, CuttingPresenter::OptimizationLeftoverAuditStats>
CuttingPresenter::collectOptimizationLeftoverStats(
    const QHash<QUuid, QVector<QUuid>>& perMachine)
{
    QHash<QUuid, OptimizationLeftoverAuditStats> stats;

    int hours = SettingsManager::instance().optimizationLeftoverAuditHours();
    QDateTime now = QDateTime::currentDateTime();

    for (auto it = perMachine.begin(); it != perMachine.end(); ++it) {

        QUuid machineId = it.key();
        const QVector<QUuid>& ids = it.value();

        OptimizationLeftoverAuditStats s;

        for (const auto& id : ids) {

            auto entryOpt = LeftoverStockRegistry::instance().findById(id);
            if (!entryOpt) {
                s.missing++;
                continue;
            }

            const auto& e = *entryOpt;

            bool isMissing = (e.notFoundCount > 0);
            bool isStale   = (e.lastSeenAt < now.addSecs(-hours * 3600));

            if (isMissing)
                s.missing++;
            else if (isStale)
                s.stale++;
            else
                s.fresh++;
        }

        stats[machineId] = s;
    }

    return stats;
}
