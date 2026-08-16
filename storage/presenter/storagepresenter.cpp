#include "storagepresenter.h"

#include <QPdfWriter>
#include <QPainter>
#include <QDir>
#include <QDateTime>

#include "common/eventlogger.h"
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
