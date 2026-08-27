#include "storagepresenter.h"

#include <QPdfWriter>
#include <QPainter>
#include <QDir>
#include <QDateTime>

#include "common/eventlogger.h"
#include "stock/utils/materialbarcodelistform_utils.h"
#include "stock/utils/stockintakeform_utils.h"
#include "stock/utils/stocklistform_utils.h"
#include "stock/utils/storageqrcodelistform_utils.h"
#include "storage/registry/storageregistry.h"
#include "common/qrcodepainter.h"          // saját QR generátor
#include "storage/utils/storage_label_utils.h"
#include "view/MainWindow.h"

StoragePresenter::StoragePresenter(MainWindow* view, QObject* parent)
    : QObject(parent), _view(view)
{
}

void StoragePresenter::exportStorageLabelPdf(const QUuid& storageId)
{
    const StorageEntry* st = StorageRegistry::instance().findById(storageId);
    if (!st) {
        zEvent("❌ Ismeretlen tárhely ID.");
        return;
    }

    QString dir = "_reports";
    QDir().mkpath(dir);

    QString path = QString("%1/StorageLabel_%2_%3.pdf")
                       .arg(dir)
                       .arg(st->barcode)
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

    StorageLabelUtils::drawStorageLabel(painter, writer, pageRect, st, true);

    painter.end();
    zEvent(QString("📄 Tárhely címke exportálva: %1").arg(path));
}

void StoragePresenter::exportMultipleLabels(const QList<StorageEntry*>& entries)
{
    if (entries.isEmpty()) {
        zEvent("⚠️ Nincs kijelölt tárhely a címkékhez.");
        return;
    }

    // zInfo("=== Tömeges címke export indul ===");
    // for (const StorageEntry* st : entries) {
    //     zInfo(QString("➡️ %1 | %2 | %3")
    //               .arg(st->id.toString())
    //               .arg(st->name)
    //               .arg(st->barcode));
    // }
    // zInfo("=== Lista vége ===");

    QString dir = "_reports";
    QDir().mkpath(dir);

    QString path = QString("%1/StorageLabels_%2.pdf")
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

    // ⭐ 5 címke egy lapon
    const int maxPerPage = 5;
    qreal cellHeight = pageRect.height() / maxPerPage;

    int count = 0;

    for (const StorageEntry* st : entries) {

        if (count > 0 && count % maxPerPage == 0) {
            writer.newPage();   // új lap minden 5 címke után
        }

        int indexOnPage = count % maxPerPage;

        QRectF cellRect(
            pageRect.left(),
            pageRect.top() + indexOnPage * cellHeight,
            pageRect.width(),
            cellHeight
            );

        StorageLabelUtils::drawStorageLabel(painter, writer, cellRect, st, false);

        count++;
    }

    painter.end();
    zEvent(QString("📄 Tömeges tárhely címkék exportálva: %1").arg(path));
}

void StoragePresenter::exportStockIntakeForm(const QUuid& storageId)
{
    const StorageEntry* st = StorageRegistry::instance().findById(storageId);
    if (!st) {
        zEvent("❌ Ismeretlen tárhely ID.");
        return;
    }

    QString dir = "_reports";
    QDir().mkpath(dir);

    QString path = QString("%1/StockIntake_%2_%3.pdf")
                       .arg(dir)
                       .arg(st->barcode)
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

    // === 1) Tárhely címke a lap tetején ===
    QRectF labelRect(
        pageRect.left(),
        pageRect.top(),
        pageRect.width(),
        pageRect.height() * 0.25
        );

    painter.save();   // 🔥 painter állapot mentése

    StorageLabelUtils::drawStorageLabel(painter, writer, labelRect, st, false);

    painter.restore(); // 🔥 painter visszaállítása → megszűnik a skálázás

    // === 2) Anyagfelvételi táblázat leftover-stílusban ===
    QRectF tableRect(
        pageRect.left(),
        pageRect.top() + pageRect.height() * 0.25,
        pageRect.width(),
        pageRect.height() * 0.75
        );

    StockIntakeFormUtils::drawStockIntakeTable(painter, tableRect);

    painter.end();
    zEvent(QString("📄 Anyagfelvételi űrlap exportálva: %1").arg(path));
}

void StoragePresenter::exportStockListPdf(const QUuid& storageId)
{
    const StorageEntry* st = StorageRegistry::instance().findById(storageId);
    if (!st) {
        zEvent("❌ Ismeretlen tárhely ID.");
        return;
    }

    // készlet lekérése
    QList<StockEntry> entries =
        StockRegistry::instance().findByStorageId(storageId);

    QString dir = "_reports";
    QDir().mkpath(dir);

    QString path = QString("%1/StockList_%2_%3.pdf")
                       .arg(dir)
                       .arg(st->barcode)
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

    // 1) Tárhely címke
    painter.save();
    QRectF labelRect(
        pageRect.left(),
        pageRect.top(),
        pageRect.width(),
        pageRect.height() * 0.25
        );
    StorageLabelUtils::drawStorageLabel(painter, writer, labelRect, st, false);
    painter.restore();

    // 2) Készletlista táblázat
    QRectF tableRect(
        pageRect.left(),
        pageRect.top() + pageRect.height() * 0.25,
        pageRect.width(),
        pageRect.height() * 0.75
        );

    StockListFormUtils::drawStockListTable(painter, tableRect, entries);

    painter.end();
    zEvent(QString("📄 Készletlista exportálva: %1").arg(path));
}

void StoragePresenter::exportMaterialBarcodeList()
{
    // 1️⃣ Anyagok összegyűjtése
    QList<QUuid> materialIds;
    for (const auto& m : MaterialRegistry::instance().readAll())
        materialIds.append(m.id);

    if (materialIds.isEmpty()) {
        zEvent("ℹ️ Nincs anyag a törzsben.");
        return;
    }

    // 2️⃣ PDF létrehozása
    QString dir = "_reports";
    QDir().mkpath(dir);

    QString path = QString("%1/MaterialBarcodeList_%2.pdf")
                       .arg(dir)
                       .arg(QDateTime::currentDateTime().toString("yyyyMMdd_HHmm"));

    QPdfWriter writer(path);
    writer.setPageSize(QPageSize(QPageSize::A4));
    writer.setResolution(300);

    QPainter painter(&writer);
    painter.setFont(QFont("Noto Sans Mono", 11));

    QRectF pageRect = writer.pageLayout().paintRectPixels(writer.resolution());

    // 3️⃣ Oldaltöréshez szükséges változók
    qreal y = pageRect.top() + 40.0;
    const qreal topMargin = 40.0;

    QFontMetrics fm(painter.font());

    const qreal textH = fm.height();      // pl. 14–18 px

    auto drawHeader = [&]() {
        QString title = QString("🏷️ Anyag Vonalkódjegyzék – %1")
                            .arg(QDateTime::currentDateTime().toString("yyyy.MM.dd HH:mm"));

        painter.drawText(QRectF(pageRect.left() + 40, y, pageRect.width(), textH),
                         Qt::AlignLeft,
                         title);
        y += textH+40;

        // táblázat fejléc
        painter.drawText(QRectF(pageRect.left() + 40, y, pageRect.width(), textH),
                         Qt::AlignLeft,
                         "External        Material + Barcode");
        y += textH + 40;

        painter.drawLine(pageRect.left() + 40, y,
                         pageRect.right() - 40, y);
        y += 10;
    };

    drawHeader();

    // 4️⃣ Sorok rajzolása több oldalon
    const qreal bcH   = 120;               // barcode magasság
    const qreal gap1  = 10;                // text → barcode gap
    const qreal gap2  = 20;               // barcode → következő sor gap

    const qreal rowHeight = textH + gap1 + bcH + gap2;

    for (const QUuid& id : materialIds) {

        if (y + rowHeight > pageRect.bottom() - 40) {
            writer.newPage();
            y = pageRect.top() + topMargin;
            drawHeader();
        }

        y = MaterialBarcodeListFormUtils::drawOneMaterialRow(
            painter,
            pageRect,
            y,
            id
            );
    }


    painter.end();
    zEvent(QString("📄 Anyag vonalkódjegyzék exportálva: %1").arg(path));
}


void StoragePresenter::exportStorageBarcodeList()
{
    // 1️⃣ Tárhelyek összegyűjtése
    QList<QUuid> storageIds;
    for (const auto& s : StorageRegistry::instance().readAll())
        storageIds.append(s.id);

    if (storageIds.isEmpty()) {
        zEvent("ℹ️ Nincs tárhely a rendszerben.");
        return;
    }

    // 2️⃣ PDF létrehozása
    QString dir = "_reports";
    QDir().mkpath(dir);

    QString path = QString("%1/StorageQrcodeList_%2.pdf")
                       .arg(dir)
                       .arg(QDateTime::currentDateTime().toString("yyyyMMdd_HHmm"));

    QPdfWriter writer(path);
    writer.setPageSize(QPageSize(QPageSize::A4));
    writer.setResolution(300);

    QPainter painter(&writer);
    painter.setFont(QFont("Noto Sans Mono", 11));

    QRectF pageRect = writer.pageLayout().paintRectPixels(writer.resolution());

    // 3️⃣ Oldaltöréshez szükséges változók
    qreal y = pageRect.top() + 40.0;
    const qreal topMargin = 40.0;

    QFontMetrics fm(painter.font());
    const qreal textH = fm.height();

    auto drawHeader = [&]() {
        QString title = QString("📦 Tárhely QR‑kód lista – %1")
                            .arg(QDateTime::currentDateTime().toString("yyyy.MM.dd HH:mm"));

        painter.drawText(QRectF(pageRect.left() + 40, y, pageRect.width(), textH),
                         Qt::AlignLeft,
                         title);
        y += textH + 40;

        painter.drawText(QRectF(pageRect.left() + 40, y, pageRect.width(), textH),
                         Qt::AlignLeft,
                         "Storage Name        Logistic Barcode + QR");
        y += textH + 40;

        painter.drawLine(pageRect.left() + 40, y,
                         pageRect.right() - 40, y);
        y += 10;
    };

    drawHeader();

    // 4️⃣ Sorok rajzolása több oldalon
    const qreal qrH   = 150;   // QR magasság
    const qreal gap1  = 10;    // text → QR gap
    const qreal gap2  = 10;    // QR → következő sor gap

    const qreal rowHeight = textH + gap1 + qrH + gap2;
    int l = 0;
    for (const QUuid& id : storageIds) {

        if (y + rowHeight > pageRect.bottom() - 40) {
            writer.newPage();
            y = pageRect.top() + topMargin;
            drawHeader();
            l=1;
        }

        l++;
        y = StorageQrcodeListFormUtils::drawOneStorageRow(
            painter,
            pageRect,
            y,
            id, l
            );
    }

    painter.end();
    zEvent(QString("📄 Tárhely QR‑kód lista exportálva: %1").arg(path));
}
