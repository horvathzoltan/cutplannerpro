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
#include "../model/registries/leftoverstockregistry.h"
#include "stock/registry/stockregistry.h"
#include "../service/cutting/optimizer/exporter.h"
#include "../common/filenamehelper.h"
#include "settings/settingsmanager.h"
#include "../model/repositories/cuttingrequestrepository.h"
#include "materials/registry/material_registry.h"
#include "../service/cutting/optimizer/optimizationlogger.h"
#include "../service/cutting/optimizer/optimizationrunner.h"
#include "../service/cutting/optimizer/optimizationviewupdater.h"
#include "../service/snapshot/inventorysnapshotbuilder.h"
#include "../service/snapshot/requestsnapshotbuilder.h"
#include "materials/utils/material_group_utils.h"
#include "service/cutting/instruction/cuttinginstructionutils.h"
//#include "service/cutting/summary/cutplansummary.h"
#include <service/cutting/summary/cutplansummarybuilder.h>
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

QVector<Cutting::Plan::CutPlan>& CuttingPresenter::getPlansRef()
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
    if (_view) {
        OptimizationViewUpdater::update(_view, _optimizerModel);
        _view->switchToCuttingPlanTab();   // ⬅️ EZT ADJUK HOZZÁ
    }

    // 3️⃣ Export (opcionális)
    //OptimizationExporter::exportPlans(model.getResult_PlansRef());

    // 4️⃣ Audit sorok előállítása
    auto sp = _view->storageAuditPresenter();
    auto am = sp->auditStateManager();

    am->setOutdated(AuditStateManager::AuditOutdatedReason::OptimizeRun);
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

        if (_view)
            _view->ShowWarningDialog(msg);

        isModelSynced = false;
        return;
    }

    // 8️⃣ Modell betöltése
    _optimizerModel.setCuttingRequests(requests);
    _optimizerModel.setInventorySnapshot(inventory);
    isModelSynced = true;

    // 9️⃣ Warningok megjelenítése (nem tiltó)
    if (!warnings.isEmpty() && _view) {
        QString msg = "Figyelmeztetések:\n\n• " + warnings.join("\n• ");
        _view->ShowWarningDialog(msg);
    }

}

bool CuttingPresenter::loadCuttingPlanFromFile(const QString& path) {
    bool loaded = CuttingRequestRepository::loadFromFile(CuttingPlanRequestRegistry::instance(), path);

    return loaded;
}

void CuttingPresenter::ExportCutPlanSummary() {

    static const QString errevent = QStringLiteral("❌ Summary export nem hajtható végre. Részletek a logban.");
    static const QString oklog = QStringLiteral("✅ Cut Plan Summary exportálva: %1");

    const auto& plans = _optimizerModel.getResult_PlansRef();

    // 1️⃣ Guard: nincs optimalizációs eredmény
    if (plans.isEmpty()) {
        if (_view)
            _view->ShowWarningDialog(
                "Nincs optimalizációs eredmény.\n"
                "A Summary export nem hajtható végre."
                );
        return;
    }

    const auto& leftovers = _optimizerModel.getResults_Leftovers();

    QString fileName = SettingsManager::instance().cuttingPlanFileName();
    QFileInfo fi(fileName);
    QString baseName = fi.completeBaseName();

    if (baseName.isEmpty()) {
        zWarning(errevent);
        zEvent("❌ Nincs Cutting Plan fájlnév — a Summary export nem hajtható végre.");
        return;
    }

    CutPlanSummary summary = CutPlanSummaryBuilder::build(plans, leftovers, baseName, _optimizerModel._fitTelemetry);

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

    QVector<Cutting::Plan::CutPlan> &cutPlans = _optimizerModel.getResult_PlansRef();

    if (cutPlans.isEmpty()) {
        if (_view)
            _view->ShowWarningDialog("Nincs optimalizációs eredmény.\n"
                                     "Előbb futtasd az Optimize műveletet.");
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


void CuttingPresenter::ExportCutInstructions()
{

    if (_machineCutsList.isEmpty()) {
        if (_view)
            _view->ShowWarningDialog("Nincs legenerált vágási utasítás.\nElőbb futtasd a Generate CutInstructions műveletet.");
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
    QMap<QUuid, QVector<const CutInstruction*>> orderedCuts2;

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

        // gépenkénti riport lekérése a for előtt
        const auto& machineReportMap = _optimizerModel.getMachineReport();
        const auto& discardedMap = _optimizerModel.getDiscardedPieces();

        for (const auto& mc : _machineCutsList) {
            const auto& rep = machineReportMap.value(mc.machineHeader.machineId);

            // géphez tartozó FAILED darabok kigyűjtése
            QVector<DiscardedPiece> failedList;
            for (const auto& dp : discardedMap) {
                if (dp.machineId == rep.machineId)
                    failedList.append(dp);
            }

            auto m = CuttingInstructionUtils::formatMachineCutsEvent(mc, rep, failedList, baseName, SettingsManager::printedLineWidth);
            orderedCuts2.insert(mc.machineHeader.machineId, m.orderedCuts);

            out << m.planTxt << "\n\n";
        }

        zEvent(QString("📄 CutInstructions exportálva: %1").arg(path));
    }

    // --- 3) LabelTable PDF ---
    {
        QString path = dir + "/" + baseName + "_CutInstructions_Labels.pdf";

        ExportCutInstructions_Labels(path, orderedCuts2);
    }
}

void CuttingPresenter::ExportCutInstructions_2()
{

    if (_machineCutsList.isEmpty()) {
        if (_view)
            _view->ShowWarningDialog("Nincs legenerált vágási utasítás.\nElőbb futtasd a Generate CutInstructions műveletet.");
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

    QMap<QUuid, QVector<const CutInstruction*>> orderedCuts2;
    // --- 1) CutInstructions_Rod.txt ---
    {
        QString path = dir + "/" + baseName + "_CutInstructions_Rod.txt";
        QFile f(path);
        if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
            zEvent("❌ Nem sikerült megnyitni a CutInstructions_Rod fájlt.");
            return;
        }

        QTextStream out(&f);
        out.setEncoding(QStringConverter::Utf8);

        // gépenkénti riport lekérése a for előtt
        const auto& machineReportMap = _optimizerModel.getMachineReport();
        const auto& discardedMap = _optimizerModel.getDiscardedPieces();

        for (const auto& mc : _machineCutsList) {
            const auto& rep = machineReportMap.value(mc.machineHeader.machineId);

            // géphez tartozó FAILED darabok kigyűjtése
            QVector<DiscardedPiece> failedList;
            for (const auto& dp : discardedMap) {
                if (dp.machineId == rep.machineId)
                    failedList.append(dp);
            }

            auto m = CuttingInstructionUtils::formatMachineCutsEvent_2(mc, rep, failedList, baseName, SettingsManager::printedLineWidth);
            orderedCuts2.insert(mc.machineHeader.machineId, m.orderedCuts);

            out << m.planTxt << "\n\n";
        }

        zEvent(QString("📄 CutInstructions_Rod exportálva: %1").arg(path));
    }


    // --- 3) LabelTable PDF ---
    {
        QString path = dir + "/" + baseName + "_CutInstructions_Labels_Rod.pdf";

        ExportCutInstructions_Labels(path, orderedCuts2);
    }
}

void CuttingPresenter::ExportCutInstructions_Labels(const QString& path, QMap<QUuid, QVector<const CutInstruction*>> orderedCuts2)
{
    //QString path = dir + "/" + baseName + "_CutInstructions_Labels.pdf";

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
    const qreal cellHeight = 300.0; // nagy, jól olvasható címke

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

        // QVector<LabelModel> labels =
        //     CuttingInstructionUtils::collectLabelModelsFromMachineCuts(mc);
        QVector<LabelModel> labels =
            CuttingInstructionUtils::collectLabelModelsFromMachineCuts_2(mc.leftoverInfo, orderedCuts2.value(mc.machineHeader.machineId));


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
        _view->refresh_InputTableFromRegistry();
    }
}


