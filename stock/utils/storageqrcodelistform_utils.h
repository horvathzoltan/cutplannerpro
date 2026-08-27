#pragma once
#include <QPainter>
#include <QString>
#include "common/qrcodepainter.h"
#include "storage/registry/storageregistry.h"

namespace StorageQrcodeListFormUtils {

inline qreal drawOneStorageRow(
    QPainter& painter,
    const QRectF& pageRect,
    qreal y,
    const QUuid& id,
    int l
    ){
    const StorageEntry* st = StorageRegistry::instance().findById(id);
    if (!st)
        return y + 160;

    const qreal leftMargin = 40.0;
    const qreal col1 = (pageRect.width() - 100) * 0.60;   // bal oldal: név + log barcode
    const qreal col2 = (pageRect.width() - 100) * 0.40;   // jobb oldal: QR

    QFontMetrics fm(painter.font());

    const qreal textH = fm.height();
    const qreal qrH   = 150;
    const qreal gap1  = 10;
    const qreal gap2  = 10;

    const qreal rowHeight = textH + gap1 + qrH + gap2;

    QString code = StorageRegistry::instance().logisticBarcode(id);
    QString name = StorageRegistry::instance().uniqueHumanName(id);

    //QString name = st->name;
//    QString code = st->barcode;

    qreal yTop = y;

    // --- Storage name (emberi név) ---
    painter.drawText(
        QRectF(leftMargin + 5, yTop + 10, col1 - 10, textH),
        Qt::AlignLeft,
        name
        );

    // --- Logistic barcode szöveg ---
    painter.drawText(
        QRectF(leftMargin + 5, yTop + 10 + textH + 4, col1 - 10, textH),
        Qt::AlignLeft,
        code
        );

    qreal ta = leftMargin + col1;
    if(l%2) ta+= 2*rowHeight;

    // --- QR-kód jobb oldalt ---
    QRectF qrRect(
        ta,
        yTop + 10,
        rowHeight-20,
        rowHeight-20
        );

    QRv4::drawQR(painter, code, qrRect);

    // --- Sor alja ---
    painter.drawLine(leftMargin, yTop + rowHeight,
                     pageRect.right() - leftMargin, yTop + rowHeight);

    return yTop + rowHeight;
}

} // namespace
