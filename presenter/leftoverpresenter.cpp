#include "leftoverpresenter.h"
#include "common/eventlogger.h"
#include "common/logger.h"
#include "service/cutting/instruction/cuttinginstructionutils.h"
#include "view/dialog/waste/leftoverreviewdialog.h"
#include "view/managers/leftovertable_manager.h"
#include "view/utils/leftoverreviewform_utils.h"
#include <QDir>
#include <QMessageBox>
#include <QPainter>
#include <QPdfWriter>
#include <QRandomGenerator>
#include <model/registries/leftoverstockregistry.h>
#include <settings/settingsmanager.h>

LeftoverPresenter::LeftoverPresenter(LeftoverTableManager* mgr)
    : _mgr(mgr)
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


void LeftoverPresenter::ExportReviewFormPdf()
{
    const int rowsPerPage = 10;

    QVector<LeftoverStockEntry> selected;

    QVector<LeftoverStockEntry> all = LeftoverStockRegistry::instance().readAll();
    QVector<LeftoverStockEntry> expired;

    const QDateTime now = QDateTime::currentDateTime();
    int daysThreshold = SettingsManager::instance().leftoverAgeThresholdDays();

    // 1) Gyűjtsük ki az összes lejártat
    for (const auto& e : all) {
        if (e.lastSeenAt.daysTo(now) > daysThreshold)
            expired.append(e);
    }

    // 2) Ha kevesebb mint 10 van, akkor mindet visszaadjuk
    if (expired.size() <= rowsPerPage) {
        selected = expired;
    } else {
        // 3) Randomizáljuk
        std::shuffle(expired.begin(), expired.end(), *QRandomGenerator::global());

        // 4) Vegyük az első 10-et
        selected = expired.mid(0, rowsPerPage);
    }

    if (selected.isEmpty()) {
        zEvent("ℹ️ Nincs olyan leftover, amely szemlére vár.");
        return;
    }

    QString dir = "_reports";
    QDir().mkpath(dir);

    QString path = QString("%1/leftover_reviewform_%2.pdf")
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

    QRectF pageRect = writer.pageLayout().paintRectPixels(writer.resolution());
    painter.setFont(QFont("Noto Sans Mono", 11));

    LeftoverReviewFormUtils::formatReviewFormPdf(
        painter,
        writer,
        pageRect,
        selected,
        rowsPerPage
        );

    painter.end();
    zEvent(QString("📄 Leftover Review Form PDF exportálva: %1").arg(path));
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

