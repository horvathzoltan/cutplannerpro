#include "leftoverpresenter.h"
#include "common/eventlogger.h"
#include "leftover/label/leftoverlabelgenerator.h"
#include "view/MainWindow.h"

#include "common/logger.h"
#include "leftover/services/bundlesplitengine.h"
#include "service/cutting/instruction/cuttinginstructionutils.h"
#include "leftover/view/dialog/leftoverreviewdialog.h"
#include "leftover/view/managers/leftovertable_manager.h"
#include "leftover/view/utils/leftoverreviewform_utils.h"
#include <QDir>
#include <QMessageBox>
#include <QPainter>
#include <QPdfWriter>
#include <QRandomGenerator>
#include "leftover/registry/leftoverstockregistry.h"
#include <settings/settingsmanager.h>
#include <leftover/audit/leftoveraudit.h>
#include <leftover/label/leftoverlabelqueue.h>
#include <leftover/substitution/leftoversubstitutionengine.h>
#include <view/dialog/textviewdialog.h>

LeftoverPresenter::LeftoverPresenter(MainWindow* view, LeftoverTableManager* mgr)
    :  _view(view), _mgr(mgr)
{}

void LeftoverPresenter::Review() {
    LeftoverReviewDialog dlg;

    while (dlg.exec() == QDialog::Accepted) {

        QString auditCode = dlg.barcode().trimmed();
        if (auditCode.isEmpty()) {
            if (!dlg.repeat()) break;
            dlg.clearBarcodeField();
            continue;
        }

        processAuditCode(auditCode);

        if (!dlg.repeat()) break;
        dlg.clearBarcodeField();
    }
}

void LeftoverPresenter::processAuditCode(const QString& auditCode)
{
    bool isPresent = auditCode.endsWith("+");
    bool isMissing = auditCode.endsWith("-");

    if (!isPresent && !isMissing) {
        QMessageBox::warning(nullptr, "Invalid code",
                             "Audit code must end with + or -");
        return;
    }

    QString original = auditCode.left(auditCode.length() - 1);

    auto entryOpt = LeftoverStockRegistry::instance().findByBarcode(original);
    if (!entryOpt) {
        QMessageBox::warning(nullptr, "Not found",
                             "No leftover found with this barcode.");
        return;
    }

    auto entry = *entryOpt;

    if (isPresent) {
        LeftoverStockRegistry::instance().markSeen(entry.entryId);
    } else {
        LeftoverStockRegistry::instance().markNotFound(entry.entryId);
    }

    // újra lekérjük a frissített entry-t
    auto updated = LeftoverStockRegistry::instance().findById(entry.entryId);
    if (updated) {
        _mgr->updateRow(*updated);
    }
}


void LeftoverPresenter::ExportLeftoverIntakeForm_Pdf()
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

void LeftoverPresenter::ExportLeftoverIntakeForm()
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
        SettingsManager::printedLineWidth, rowsPerPage);

    zEvent(QString("📄 Leftover Intake Form exportálva: %1").arg(path));
}

/*
 * Review finctions
 */

// void LeftoverPresenter::ExportReviewFormPdf_old()
// {
//     const int rowsPerPage = 10;

//     QVector<LeftoverStockEntry> selected;

//     QVector<LeftoverStockEntry> all = LeftoverStockRegistry::instance().readAll();
//     QVector<LeftoverStockEntry> expired;

//     const QDateTime now = QDateTime::currentDateTime();
//     int daysThreshold = SettingsManager::instance().leftoverAgeThresholdDays();

//     // 1) Gyűjtsük ki az összes lejártat
//     for (const auto& e : all) {
//         if (e.lastSeenAt.daysTo(now) > daysThreshold)
//             expired.append(e);
//     }

//     // 2) Ha kevesebb mint 10 van, akkor mindet visszaadjuk
//     if (expired.size() <= rowsPerPage) {
//         selected = expired;
//     } else {
//         // 3) Randomizáljuk
//         std::shuffle(expired.begin(), expired.end(), *QRandomGenerator::global());

//         // 4) Vegyük az első 10-et
//         selected = expired.mid(0, rowsPerPage);
//     }

//     if (selected.isEmpty()) {
//         zEvent("ℹ️ Nincs olyan leftover, amely szemlére vár.");
//         return;
//     }

//     QString dir = "_reports";
//     QDir().mkpath(dir);

//     QString path = QString("%1/leftover_reviewform_%2.pdf")
//                        .arg(dir)
//                        .arg(QDateTime::currentDateTime().toString("yyyyMMdd_HHmm"));

//     QPdfWriter writer(path);
//     writer.setPageSize(QPageSize(QPageSize::A4));
//     writer.setResolution(300);

//     QPainter painter(&writer);
//     if (!painter.isActive()) {
//         zEvent("❌ Nem sikerült megnyitni a PDF fájlt.");
//         return;
//     }

//     QRectF pageRect = writer.pageLayout().paintRectPixels(writer.resolution());
//     painter.setFont(QFont("Noto Sans Mono", 11));

//     LeftoverReviewFormUtils::formatReviewFormPdf(
//         painter,
//         writer,
//         pageRect,
//         selected,
//         rowsPerPage
//         );

//     painter.end();
//     zEvent(QString("📄 Leftover Review Form PDF exportálva: %1").arg(path));
// }


// void LeftoverPresenter::ExportReviewFormPdf()
// {
//     int daysThreshold = SettingsManager::instance().leftoverAgeThresholdDays();

//     // ÚJ: irányított audit modul hívása
//     QVector<LeftoverStockEntry> list =
//         LeftoverAudit::collectExpired(daysThreshold);

//     if (list.isEmpty()) {
//         zEvent("ℹ️ Nincs olyan leftover, amely szemlére vár.");
//         return;
//     }

//     // ÚJ: egységes PDF export
//     const int rowsPerPage = 20;

//     QString dir = "_reports";
//     QDir().mkpath(dir);

//     QString path = QString("%1/leftover_expired_%2.pdf")
//                        .arg(dir)
//                        .arg(QDateTime::currentDateTime().toString("yyyyMMdd_HHmm"));

//     QPdfWriter writer(path);
//     writer.setPageSize(QPageSize(QPageSize::A4));
//     writer.setResolution(300);

//     QPainter painter(&writer);
//     if (!painter.isActive()) {
//         zEvent("❌ Nem sikerült megnyitni a PDF fájlt.");
//         return;
//     }

//     QRectF pageRect = writer.pageLayout().paintRectPixels(writer.resolution());
//     painter.setFont(QFont("Noto Sans Mono", 11));

//     LeftoverReviewFormUtils::formatReviewFormPdf(
//         painter,
//         writer,
//         pageRect,
//         list,
//         rowsPerPage
//         );

//     painter.end();
//     zEvent(QString("📄 Leftover Expired Audit PDF exportálva: %1").arg(path));
// }

void LeftoverPresenter::ExportReviewFormPdf()
{
    int daysThreshold = SettingsManager::instance().leftoverAgeThresholdDays();

    // 1) Gyűjtjük a régen látott leftovereket
    QVector<LeftoverStockEntry> list =
        LeftoverAudit::collectExpired(daysThreshold);

    if (list.isEmpty()) {
        zEvent("ℹ️ Nincsenek leftoverek.");
        return;
    }

    QVector<LeftoverStockEntry> list_filtered=filter(list);

    if (list_filtered.isEmpty()) {
        zEvent("ℹ️ Nincs olyan leftover, amely szemlére vár.");
        return;
    }

    QVector<LeftoverStockEntry> list_shuffled = shuffle(list_filtered, 10);

    // 3) Egységes PDF export
    exportAuditPdf(list_shuffled, "leftover_expired");
}

QVector<LeftoverStockEntry> LeftoverPresenter::shuffle(QVector<LeftoverStockEntry>& list,
                                                       int rowsPerPage)
{
    if (list.size() <= rowsPerPage)
        return list;

    std::shuffle(list.begin(), list.end(), *QRandomGenerator::global());
    return list.mid(0, rowsPerPage);
}

QVector<LeftoverStockEntry> LeftoverPresenter::filter(const QVector<LeftoverStockEntry>& list)
{
    QVector<LeftoverStockEntry> list_filtered;
        const QDateTime now = QDateTime::currentDateTime();

    for(auto&a:list){
        if(a.materialType().value == MaterialType::Type::Steel)
            continue;
        if(!a.materialBarcode().startsWith("NP-"))
            continue;

        // csak az 1 óránál régebben látottak
       if (a.lastSeenAt.secsTo(now) < 3600)
           continue;

        list_filtered<<a;
    }
    return list_filtered;
}

void LeftoverPresenter::ExportStorageAuditPdf(
    const QUuid& storageId,
    const QVector<QUuid>& materialIds)
{
    int daysThreshold = SettingsManager::instance().leftoverAgeThresholdDays();

    QVector<LeftoverStockEntry> list =
        LeftoverAudit::collectStorageAudit(storageId, materialIds, daysThreshold);

    exportAuditPdf(list, "leftover_storageaudit");
}

void LeftoverPresenter::exportAuditPdf(
    const QVector<LeftoverStockEntry>& list,
    const QString& title)
{
    if (list.isEmpty()) {
        zEvent("ℹ️ Nincs auditálható leftover.");
        return;
    }

    const int rowsPerPage = 15;

    QString dir = "_reports";
    QDir().mkpath(dir);

    QString path = QString("%1/%2_%3.pdf")
                       .arg(dir)
                       .arg(title)
                       .arg(QDateTime::currentDateTime().toString("yyyyMMdd_HHmm"));

    QPdfWriter writer(path);
    writer.setPageSize(QPageSize(QPageSize::A4));
    writer.setResolution(300);

    QPainter painter(&writer);
    if (!painter.isActive()) {
        zEvent("❌ Nem sikerült megnyitni a PDF fájlt.");
        return;
    }
    QPageLayout layout = writer.pageLayout();
    QRectF full  = layout.fullRectPixels(writer.resolution());
    QRectF paint = layout.paintRectPixels(writer.resolution());
    // valós nem nyomtatható jobb oldali margó
    qreal rightNonPrintable = full.right() - paint.right();
    qreal printerSafeMarginRight = rightNonPrintable + 5.0;// adjunk hozzá 5 px biztonsági ráhagyást
    // KORRIGÁLT pageRect – ezt kell használni minden rajzoláshoz
    QRectF pageRect(
        paint.left(),
        paint.top(),
        paint.width() - printerSafeMarginRight,
        paint.height()
        );

    painter.setFont(QFont("Noto Sans Mono", 7));

    // ⭐ Többoldalas logika
    int total = list.size();
    int pageCount = (total + rowsPerPage - 1) / rowsPerPage;

    for (int page = 0; page < pageCount; ++page) {

        int start = page * rowsPerPage;
        int end   = qMin(start + rowsPerPage, total);

        QVector<LeftoverStockEntry> slice = list.mid(start, end - start);

        LeftoverReviewFormUtils::formatReviewFormPdf(
            painter,
            writer,
            pageRect,
            slice,
            rowsPerPage
            );

        if (page < pageCount - 1)
            writer.newPage();   // ⭐ Új lap
    }

    painter.end();
    zEvent(QString("📄 Audit PDF exportálva: %1").arg(path));
}


void LeftoverPresenter::ExportOptimizationLeftoverAuditPdf(
    const QHash<QUuid, QVector<QUuid>>& perMachine)
{
    for (auto it = perMachine.begin(); it != perMachine.end(); ++it) {

        QUuid machineId = it.key();
        QVector<QUuid> leftoverIds = it.value();

        QVector<LeftoverStockEntry> list =
            LeftoverAudit::collectUsedLeftovers(leftoverIds);

        auto mac = CuttingMachineRegistry::instance().findById(machineId);
        QString machineName = mac ? mac->name : "???";

        exportAuditPdf(list, QString("leftover_opt_audit_%1").arg(machineName));
    }
}

void LeftoverPresenter::applyBundleSplit(const BundleSplitResult& r)
{
    // 1️⃣ eredeti leftover frissítése
    update_LeftoverStockEntry(r.updatedOriginal);

    // 2️⃣ új leftoverek hozzáadása
    for (const auto& e : r.newLeftovers)
    {
        add_LeftoverStockEntry(e);

        LabelModel lm = LeftoverLabelGenerator::makeBundleLeftoverLabel(
            e,
            r.updatedOriginal.barcode
            );

        LeftoverLabelQueue::instance().append(lm);
    }
}

bool LeftoverPresenter::remove_LeftoverStockEntry(const QUuid& entryId) {
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

void LeftoverPresenter::add_LeftoverStockEntry(const LeftoverStockEntry& req) {
    LeftoverStockRegistry::instance().registerEntry(req);
    if(_view){
        _view->addRow_LeftoversTable(req);
    }
    auto sp = _view->storageAuditPresenter();
    auto m = sp->auditStateManager();

    m->setOutdated(AuditStateManager::AuditOutdatedReason::LeftoverChanged);
}



void LeftoverPresenter::update_LeftoverStockEntry(const LeftoverStockEntry& updated) {
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

/**/
void LeftoverPresenter::ExportOptimizationLeftoverAudit(
    const QHash<QUuid, CuttingPresenter::OptimizationLeftoverAuditStats>& stats)
{
    //
    // 1️⃣ Audit statisztika
    //
    //auto perMachine = _cuttingPresenter->collectUsedLeftoversFromPlans();
    //auto stats = _cuttingPresenter->collectOptimizationLeftoverStats(perMachine);

    //auto stats = CuttingPresenter::instance()->collectOptimizationLeftoverStats(perMachine);

    bool needAudit = false;

    for (auto it = stats.begin(); it != stats.end(); ++it) {
        const auto& s = it.value();
        if (s.missing > 0 || s.stale > 0) {
            needAudit = true;
            break;
        }
    }

    if (!needAudit) {
        QMessageBox::information(nullptr,
                                 "Optimalization Leftover Audit",
                                 "✅ Minden leftover friss.\nA vágás indítható.");
        return;
    }

    //
    // 2️⃣ Szűrt leftover lista: csak auditálandók
    //
    QHash<QUuid, QVector<QUuid>> filtered;

    for (auto it = stats.begin(); it != stats.end(); ++it) {
        QUuid machineId = it.key();
        const auto& s = it.value();

        QVector<QUuid> ids;

        for (const auto& id : s.missingIds)
            ids.append(id);

        for (const auto& id : s.staleIds)
            ids.append(id);

        if (!ids.isEmpty())
            filtered[machineId] = ids;
    }

    //
    // 3️⃣ Részletes statisztika megjelenítése
    //
    QString msg;

    for (auto it = stats.begin(); it != stats.end(); ++it) {
        QUuid machineId = it.key();
        const auto& s = it.value();

        auto mach = CuttingMachineRegistry::instance().findById(machineId);
        QString machName = mach ? mach->name : "Ismeretlen gép";

        msg += QString("Gép: %1\n").arg(machName);
        msg += QString("  ❌ Eltűnt: %1\n").arg(s.missing);
        msg += QString("  🕒 Lejárt: %1\n").arg(s.stale);
        msg += QString("  ✅ Friss: %1\n\n").arg(s.fresh);
    }

    QMessageBox::warning(nullptr,
                         "Optimalization Leftover Audit",
                         msg);

    //
    // 4️⃣ PDF export
    //
    ExportOptimizationLeftoverAuditPdf(filtered);
}


void LeftoverPresenter::ExportSubstitutionAuditPdf(
    const QVector<LeftoverStockEntry>& list)
{
    exportAuditPdf(list, "leftover_opt_substitution_audit");
}

/**/

// Iteratív Leftover Substitution Audit Engine

// ----------------------------------------------------
// Iteratív leftover audit kör
// ----------------------------------------------------
//
// Cél:
//  - kiválasztjuk azokat a leftovereket, amelyek
//    a tervekből használatban vannak, de minőségük
//    (lastSeenAt / notFoundCount) alapján bizonytalan,
//  - ezekhez keresünk max. 3 helyettesítő jelöltet,
//    azonos materialBarCode + elég hossz,
//  - mindezt PDF-be exportáljuk auditálásra,
//    majd a Review dialógussal finomítjuk a minőséget.
//
void LeftoverPresenter::runIterativeLeftoverAuditRound()
{
    auto cuttingPresenter = _view->cuttingPresenter();
    if (!cuttingPresenter) {
        QMessageBox::warning(nullptr,
                             "Iteratív audit",
                             "Nincs CuttingPresenter példány.");
        return;
    }

    // 1) Tervekből használt leftoverek
    QHash<QUuid, QVector<QUuid>> perMachine =
        cuttingPresenter->collectUsedLeftoversFromPlans();

    QVector<LeftoverStockEntry> allEntries =
        LeftoverStockRegistry::instance().readAll();

    QHash<QUuid, LeftoverStockEntry> byId;
    for (const auto& e : allEntries)
        byId.insert(e.entryId, e);

    QVector<LeftoverStockEntry> targets;
    QHash<QUuid, QVector<LeftoverStockEntry>> candidateSets;

    QSet<QUuid> usedAsCandidate;

    const QDateTime now = QDateTime::currentDateTime();

    auto isFresh = [&](const LeftoverStockEntry& e) {
        int hours = SettingsManager::instance().optimizationLeftoverAuditHours();
        QDateTime now = QDateTime::currentDateTime();

        if (e.notFoundCount > 0)
            return false;

        if (!e.lastSeenAt.isValid())
            return false;

        return e.lastSeenAt >= now.addSecs(-hours * 3600);
    };


    // 2) Target + jelöltek
    for (auto it = perMachine.constBegin(); it != perMachine.constEnd(); ++it) {
        for (const QUuid& id : it.value()) {

            if (!byId.contains(id))
                continue;

            const auto& target = byId[id];

            // minőségi státusz meghatározása – de NEM szűrünk ki miatta
            bool isMissing = (target.notFoundCount > 0);
            bool isFreshTarget = isFresh(target);
            bool isStale = (!isMissing && !isFreshTarget);

            // minden leftover szerepeljen a target listában
            targets.append(target);

            // ha a target friss → nem kell helyettesítés
            if (isFreshTarget) {
                continue;
            }

            QVector<LeftoverStockEntry> cands;
            for (const auto& cand : allEntries) {

                if (cand.entryId == target.entryId)
                    continue;

                if (cand.materialBarcode() != target.materialBarcode())
                    continue;

                if (cand.availableLength_mm < target.availableLength_mm)
                    continue;

                // vágandó leftovereket NE ajánljuk jelöltként
                bool isUsedInPlans = false;
                for (auto pit = perMachine.constBegin(); pit != perMachine.constEnd(); ++pit) {
                    if (pit.value().contains(cand.entryId)) {
                        isUsedInPlans = true;
                        break;
                    }
                }
                if (isUsedInPlans)
                    continue;

                // ha már jelöltként felhasználtuk → nem lehet újra jelölt
                if (usedAsCandidate.contains(cand.entryId))
                    continue;

                //bool candIsFresh = isFresh(cand);
                if (cand.isMissing())
                    continue;

                if (cand.isOld())
                    continue;


                cands.append(cand);
                usedAsCandidate.insert(cand.entryId);
            }


            std::sort(cands.begin(), cands.end(),
                      [](const auto& a, const auto& b){
                          if (a.notFoundCount != b.notFoundCount)
                              return a.notFoundCount < b.notFoundCount;
                          if (a.lastSeenAt.isValid() && b.lastSeenAt.isValid())
                              return a.lastSeenAt > b.lastSeenAt;
                          return a.availableLength_mm < b.availableLength_mm;
                      });

            if (cands.size() > 3)
                cands.resize(3);

            candidateSets.insert(target.entryId, cands);
        }
    }

    QString summary = buildCompactSummary_2(targets, candidateSets);
    //zInfo(summary);

    ExportIterativeAuditPdf(summary, targets, candidateSets);

    TextViewDialog dlg(_view);
    dlg.setWindowTitle("LEFTOVER SUBSTITUTION SUMMARY");
    dlg.setText(summary);
    dlg.exec();
}


QString LeftoverPresenter::buildSubstitutionSummary(
    const QVector<LeftoverStockEntry>& targets,
    const QHash<QUuid, QVector<LeftoverStockEntry>>& candidateSets)
{
    auto isFresh = [&](const LeftoverStockEntry& e) {
        int hours = SettingsManager::instance().optimizationLeftoverAuditHours();
        QDateTime now = QDateTime::currentDateTime();

        if (e.notFoundCount > 0)
            return false;

        if (!e.lastSeenAt.isValid())
            return false;

        return e.lastSeenAt >= now.addSecs(-hours * 3600);
    };


    QString out;

    QString dateStr = QDateTime::currentDateTime().toString("yyyy.MM.dd HH:mm");
    QString planIdStr = SettingsManager::instance().planIdStr();

    // --- fejlécek ---
    out += "📄Iteratív Leftover Substitution Audit \n";
    out += QString("CutPlan: %1").arg(planIdStr)+"\n";
    out += QString("📅 Dátum: %1").arg(dateStr)+"\n\n";

    // anyagonként csoportosítjuk
    QMap<QUuid, QVector<const LeftoverStockEntry*>> byMaterial;
    for (const auto& t : targets)
        byMaterial[t.materialId].append(&t);

    for (auto it = byMaterial.begin(); it != byMaterial.end(); ++it) {

        //out += QString("Anyag: %1 \n\n").arg(it.key());
        auto* mat = MaterialRegistry::instance().findById(it.key());

        QString matName = mat?mat->toReportLabel():"?";
        QString header = QString("Anyag: %1").arg(matName);

        out += header + "\n";
        out += QString(header.length(), QChar(0x2500)) + "\n\n";
        // 0x2500 = "─" (box drawing light horizontal)

        for (const LeftoverStockEntry* t : it.value()) {

            // TARGET sor – minőségi státusz
            bool isMissing = (t->notFoundCount > 0);
            bool isFreshTarget = isFresh(*t);
            bool isStale = (!isMissing && !isFreshTarget);


            QString tStatus;
            if (isMissing)
                tStatus = "[NINCS MEG]";
            else if (isStale)
                tStatus = "[RÉGI]";
            else
                tStatus = "[MEGVAN]";

            out += QString("%1 (%2 mm) %3\n")
                       .arg(t->barcode)
                       .arg(t->availableLength_mm)
                       .arg(tStatus);


            // helyettesítés végrehajthatósága CSAK a jelöltek minőségétől függ
            bool canExecute = false;

            const auto& cands = candidateSets.value(t->entryId);
            for (const auto& c : cands) {
                bool cIsFresh = isFresh(c);
                if (cIsFresh) {
                    canExecute = true;
                    break;
                }
            }


            // target friss → nem kell helyettesíteni
            if (isFreshTarget) {
                out += "    [HELYETTESÍTÉS NEM SZÜKSÉGES]\n";
            }
            else {
                if (canExecute)
                    out += "    [HELYETTESÍTÉS VÉGREHAJTHATÓ]\n";
                else
                    out += "    [HELYETTESÍTÉS NEM VÉGREHAJTHATÓ]\n";
            }



            if (cands.isEmpty()) {
                out += "    → (nincs jelölt)\n\n";
                continue;
            }

            QStringList candParts;
            candParts.reserve(cands.size());

            for (const auto& c : cands) {

                bool isMissing = (c.notFoundCount > 0);
                bool candIsFresh = isFresh(c);
                bool isStale = (!isMissing && !candIsFresh);


                QString status;
                if (isMissing)
                    status = "[NINCS MEG]";
                else if (isStale)
                    status = "[RÉGI]";
                else
                    status = "[MEGVAN]";

                candParts << QString("%1 (%2 mm) %3")
                                 .arg(c.barcode)
                                 .arg(c.availableLength_mm)
                                 .arg(status);
            }


            out += QString("    → %1\n\n")
                       .arg(candParts.join(", "));
        }

        out += "\n";
    }

    return out;
}


QString LeftoverPresenter::buildCompactSummary_2(
    const QVector<LeftoverStockEntry>& targets,
    const QHash<QUuid, QVector<LeftoverStockEntry>>& candidateSets)
{
    QString out;

    QString dateStr =
        QDateTime::currentDateTime().toString("yyyy.MM.dd HH:mm");

    QString planIdStr =
        SettingsManager::instance().planIdStr();

    out += QString("📄 Iteratív Leftover Audit — %1\n")
               .arg(planIdStr);

    out += QString("📅 %1\n\n")
               .arg(dateStr);


    constexpr int BarcodeW  = 14;
    constexpr int LengthW   = 6;
    constexpr int StateW    = 10;
    constexpr int CandidateW = 14;


    auto formatCandidate = [](const LeftoverStockEntry& c)
    {
        return QString("%1 (%2,%3)")
        .arg(c.barcode)
            .arg(c.availableLength_mm)
            .arg(AgeStateUtils::toShortCode(c.ageState()));
    };

    QMap<QUuid, QVector<const LeftoverStockEntry*>> byMaterial;

    for (const auto& t : targets)
        byMaterial[t.materialId].append(&t);


    for (auto it = byMaterial.begin();
         it != byMaterial.end();
         ++it)
    {
        const MaterialMaster* mat =
            MaterialRegistry::instance().findById(it.key());

        QString matName =
            mat
                ? mat->toReportLabel()
                : "?";

        out += QString("📦 Anyag: %1\n")
                   .arg(matName);

        QString header =
            QString("%1 | %2 | %3 | %4 | %5 | %6")
                .arg("Barcode",       -BarcodeW)
                .arg("Hossz",         -LengthW)
                .arg("Állapot",       -StateW)
                .arg("Helyettesítő1", -CandidateW)
                .arg("Helyettesítő2", -CandidateW)
                .arg("Helyettesítő3", -CandidateW);

        out += header + "\n";
        out += QString(header.length(), '-') + "\n";

        for (const LeftoverStockEntry* t : it.value())
        {
            QString cand1 = "—";
            QString cand2 = "—";
            QString cand3 = "—";

            const auto cands =
                candidateSets.value(t->entryId);

            if (cands.size() > 0)
                cand1 = formatCandidate(cands[0]);

            if (cands.size() > 1)
                cand2 = formatCandidate(cands[1]);

            if (cands.size() > 2)
                cand3 = formatCandidate(cands[2]);

            out += QString("%1 | %2 | %3 | %4 | %5 | %6\n")
                       .arg(t->barcode, -BarcodeW)
                       .arg(
                           QString::number(
                               t->availableLength_mm),
                           -LengthW)
                       .arg(
                           AgeStateUtils::toDisplayString(
                               t->ageState()),
                           -StateW)
                       .arg(cand1, -CandidateW)
                       .arg(cand2, -CandidateW)
                       .arg(cand3, -CandidateW);
        }

        out += "\n";
    }

    return out;
}


// ----------------------------------------------------
// Iteratív audit PDF export
// ----------------------------------------------------
//
// Egyszerű megközelítés:
//  - a cél leftoverek + jelöltek egy közös listába kerülnek,
//  - a meglévő exportAuditPdf() helperrel PDF-be írjuk.
//  - a helyettesítési logika a kódban ismeri a mappinget
//    (candidateSets), az operátor pedig minden itt szereplő
//    vonalkódot auditálhat.
//
void LeftoverPresenter::ExportIterativeAuditPdf(
    const QString& summary,
    const QVector<LeftoverStockEntry>& targets,
    const QHash<QUuid, QVector<LeftoverStockEntry>>& candidateSets)
{
    QString dir = "_reports";
    QDir().mkpath(dir);

    QString path = QString("%1/Iteratív leftover audit_%2.pdf")
                       .arg(dir)
                       .arg(QDateTime::currentDateTime().toString("yyyyMMdd_HHmm"));

    QPdfWriter writer(path);
    writer.setPageSize(QPageSize(QPageSize::A4));
    writer.setResolution(300);

    QPainter painter(&writer);
    if (!painter.isActive()) {
        zEvent("❌ Nem sikerült megnyitni a PDF fájlt.");
        return;
    }

    QPageLayout layout = writer.pageLayout();
    QRectF full  = layout.fullRectPixels(writer.resolution());
    QRectF paint = layout.paintRectPixels(writer.resolution());
    // valós nem nyomtatható jobb oldali margó
    qreal rightNonPrintable = full.right() - paint.right();
    qreal printerSafeMarginRight = rightNonPrintable + 5.0;// adjunk hozzá 5 px biztonsági ráhagyást
    // KORRIGÁLT pageRect – ezt kell használni minden rajzoláshoz
    QRectF pageRect(
        paint.left(),
        paint.top(),
        paint.width() - printerSafeMarginRight,
        paint.height()
        );

    painter.setFont(QFont("Noto Sans Mono", 10));

    // --- Fejléc ---
    // painter.drawText(pageRect, Qt::AlignLeft,
    //                  "=== Iteratív Leftover Substitution Audit ===\n\n");


    // --- Summary ---
   // QString summary = buildSubstitutionSummary(targets, candidateSets);
    painter.drawText(pageRect, Qt::AlignLeft | Qt::TextWordWrap, summary);

    writer.newPage();

    // --- Audit blokkok ---
    qreal yOffset = 0;

    painter.setFont(QFont("Noto Sans Mono", 7));

    auto draw = [&](const LeftoverStockEntry& e){
        painter.save();
        painter.translate(0, yOffset);

        qreal used = LeftoverReviewFormUtils::drawAuditBlock(painter, pageRect, e);
        yOffset += used;

        painter.restore();

        if (yOffset + used > pageRect.height() - 200) {
            writer.newPage();
            yOffset = 0;
        }
    };

    for (const auto& t : targets) {
        if(t.barcode.toLower()=="rsm-a-140"){
            zInfo("breke");
        }
        if (!t.isFresh()) {
            draw(t);
        }

        for (const auto& c : candidateSets.value(t.entryId)) {
            if(c.barcode.toLower()=="rsm-a-140"){
                zInfo("keke");
            }

            if (!c.isFresh()) {
                draw(c);
            }
        }
    }


    painter.end();
    zInfo(QString("📄 Iteratív Audit PDF exportálva: %1").arg(path));
}






