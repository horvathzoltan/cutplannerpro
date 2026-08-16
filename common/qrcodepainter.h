#pragma once
#include <QPainter>
#include <QPainterPath>
#include <QRectF>
#include <QString>

#include "common/emojihelper.h"
#include "common/qrcodegen/qrcodegen.hpp"   // amit már betettél a projektbe

namespace QRv4
{

inline void drawQR(QPainter& p, const QString& text, const QRectF& rect)
{
    // 1) Szabványos QR-kód generálása (LOW ECC)
    QByteArray ba = text.toUtf8();
    auto qr = qrcodegen::QrCode::encodeText(
        ba.constData(),
        qrcodegen::QrCode::Ecc::LOW
        );

    // 2) Mátrix méret és skálázás
    int size = qr.getSize();
    qreal scaleX = rect.width()  / size;
    qreal scaleY = rect.height() / size;
    qreal scale  = std::min(scaleX, scaleY);

    // 3) Rajzolás
    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            if (qr.getModule(x, y)) {
                p.fillRect(rect.left() + x * scale,
                           rect.top()  + y * scale,
                           scale,
                           scale,
                           Qt::black);
            }
        }
    }
}

inline QPointF computeOpticalCenter(const QImage& img)
{
    qint64 sumY = 0;
    qint64 sumA = 0;

    for (int y = 0; y < img.height(); ++y) {
        const QRgb* line = reinterpret_cast<const QRgb*>(img.scanLine(y));
        for (int x = 0; x < img.width(); ++x) {
            int a = qAlpha(line[x]);
            if (a > 0) {
                sumY += y * a;
                sumA += a;
            }
        }
    }

    if (sumA == 0)
        return QPointF(img.width()/2.0, img.height()/2.0);

    qreal cy = (qreal)sumY / (qreal)sumA;
    qreal cx = img.width() / 2.0;

    return QPointF(cx, cy);
}


inline void drawEmojiInCircle(QPainter& p, const QRectF& centerRect, const QString& emoji)
{
    QPixmap px = EmojiHelper::loadEmoji(emoji, static_cast<int>(centerRect.width()));
    QImage img = px.toImage().convertToFormat(QImage::Format_ARGB32);

    QSize s = img.size();

    // Kör közepe
    qreal cx = centerRect.center().x();
    qreal cy = centerRect.center().y();

    // Optikai középpont
    QPointF oc = computeOpticalCenter(img);

    // Kör sugara
    qreal r = qMax(s.width(), s.height()) / 2.0 * 1.15;

    // Fehér kör
    p.setPen(Qt::NoPen);
    p.setBrush(Qt::white);
    p.drawEllipse(QPointF(cx, cy), r, r);

    // Körvonal
    p.setPen(QPen(Qt::gray, 4));
    p.setBrush(Qt::NoBrush);
    p.drawEllipse(QPointF(cx, cy), r, r);

    // Emoji optikai középre igazítása
    QRectF emojiRect(
        cx - oc.x(),
        cy - oc.y(),
        s.width(),
        s.height()
        );

    p.drawPixmap(emojiRect.toRect(), px);
}




inline void drawQR_withCenterEmoji(QPainter& p, const QString& text, const QRectF& rect, const QString& emoji)
{
    QByteArray ba = text.toUtf8();
    auto qr = qrcodegen::QrCode::encodeText(
        ba.constData(),
        qrcodegen::QrCode::Ecc::HIGH   // fontos!
        );

    int size = qr.getSize();
    qreal scale = rect.width() / size;

    // QR modulok
    p.setBrush(Qt::black);
    p.setPen(Qt::NoPen);

    for (int y = 0; y < size; ++y)
        for (int x = 0; x < size; ++x)
            if (qr.getModule(x, y))
                p.drawRect(rect.left() + x * scale,
                           rect.top()  + y * scale,
                           scale, scale);

    // Emoji helye
    const int emojiSize = 120;
    QRectF centerRect(rect.center().x() - emojiSize/2,
                      rect.center().y() - emojiSize/2,
                      emojiSize,
                      emojiSize);

    drawEmojiInCircle(p, centerRect, emoji);
}




} // namespace QRv4
