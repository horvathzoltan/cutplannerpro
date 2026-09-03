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

    const int rowsPerPage = 10;

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

    QRectF pageRect = writer.pageLayout().paintRectPixels(writer.resolution());
    painter.setFont(QFont("Noto Sans Mono", 11));

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

