#pragma once
#include <QDateTime>
#include <QPainter>
#include <QString>
#include "materials/registry/material_registry.h"
#include "common/barcodepainter.h"

namespace MaterialBarcodeListFormUtils {

inline qreal drawOneMaterialRow(
    QPainter& painter,
    const QRectF& pageRect,
    qreal y,
    const QUuid& id
    ){
    const qreal leftMargin = 40.0;
    const qreal col1 = (pageRect.width() - 80) * 0.20;
    const qreal col2 = (pageRect.width() - 80) * 0.80;


    QFontMetrics fm(painter.font());

    const qreal textH = fm.height();      // pl. 14–18 px
    const qreal bcH   = 120;               // barcode magasság
    const qreal gap1  = 10;                // text → barcode gap
    const qreal gap2  = 20;               // barcode → következő sor gap

    const qreal rowHeight = textH + gap1 + bcH + gap2;

    const MaterialMaster* master = MaterialRegistry::instance().findById(id);
    if (!master)
        return y + rowHeight;

    QString ext = master->externalCode;
    if (ext.length() > 12)
        ext = ext.left(12) + "…";

    QString name = master->toReportLabel();
    QString barcode = master->barcode;

    qreal yTop = y;

    // --- External code ---
    painter.drawText(
        QRectF(leftMargin + 5, yTop + 10, col1 - 10, textH),
        Qt::AlignLeft,
        ext
        );

    // --- Material name ---
    painter.drawText(
        QRectF(leftMargin + col1 + 5, yTop + 10, col2 - 10, textH),
        Qt::AlignLeft,
        name
        );

    // --- Barcode (középre igazítva a cellán belül) ---
    if (!barcode.isEmpty()) {

        QRectF bcRect(
            leftMargin + col1 + 5+60,
            yTop + 10 + textH + 8,   // 🔥 fix pozíció a cellán belül
            col2 - 10-60,
            bcH
            );

        BarcodePainter::drawCode128(painter, barcode, bcRect);
    }

    // --- Sor alja ---
    painter.drawLine(leftMargin, yTop + rowHeight,
                     pageRect.right() - leftMargin, yTop + rowHeight);

    return yTop + rowHeight;
}


} // namespace
