#pragma once
#include <QPainter>
#include <QPdfWriter>
#include <QRectF>
#include <QString>

#include "storage/registry/storageregistry.h"
#include "common/qrcodepainter.h"

namespace StorageLabelUtils {

inline void drawStorageLabel(QPainter& painter,
                             QPdfWriter& writer,
                             const QRectF& pageRect,
                             const StorageEntry* st, bool isHearth)
{
    const qreal margin = 90.0;   // <<< nagyobb margó

    // --- Vágási keret ---
    QRectF cutRect = pageRect.adjusted(60, 60, -60, -60);   // <<< nagyobb keret
    painter.setPen(QPen(Qt::black, 3));
    //painter.drawRect(cutRect);

    // --- Felső sor: Polc neve ---
    painter.setPen(QPen(Qt::black, 2));
    painter.setFont(QFont("Noto Sans Mono", 34, QFont::Bold));

    QRectF nameRect(cutRect.left() + margin,
                    cutRect.top()  + margin,
                    cutRect.width() * 0.55,   // bal blokk szélessége
                    150);

    painter.drawText(nameRect, Qt::AlignLeft | Qt::AlignVCenter, st->name);

    // --- Alsó sor: logisztikai barcode szöveg ---
    painter.setFont(QFont("Noto Sans Mono", 24));

    QString logistic = StorageRegistry::instance().logisticBarcode(st->id);

    QRectF barcodeRect(cutRect.left() + margin,
                       nameRect.bottom() + 80,
                       cutRect.width() * 0.65,
                       80);

    painter.drawText(barcodeRect, Qt::AlignLeft | Qt::AlignVCenter, logistic);

    // --- QR-kód jobbra ---
    QRectF qrRect(
        cutRect.right() - 450 - margin,   // jobbra tolva
        nameRect.top(),                   // Polc felirat tetejéhez igazítva
        450,                              // QR méret (közepes)
        450
        );

    if(isHearth)
        QRv4::drawQR_withCenterEmoji(painter, logistic, qrRect, "❤");
    else
        QRv4::drawQR(painter, logistic, qrRect);

    // --- Vágóvonal a QR-kód alatt ---
    painter.setPen(QPen(Qt::black, 2, Qt::DashLine));

    qreal lineY = qrRect.bottom() + margin;   // QR aljától 30px

    painter.drawLine(
        QPointF(cutRect.left(),  lineY),
        QPointF(cutRect.right(), lineY)
        );
}

} // namespace StorageLabelUtils
