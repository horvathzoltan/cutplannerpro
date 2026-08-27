#pragma once

#include <QDateTime>
#include <QPainter>
#include <QPdfWriter>


namespace StockIntakeFormUtils {

inline void drawStockIntakeTable(
    QPainter& painter,
    const QRectF& pageRect
    ){
    // === leftover-stílusú paraméterek ===
    QFontMetrics fm(painter.font());
    qreal lineH = fm.height();          // ugyanaz a cellamagasság
    const qreal topMargin = 40.0;       // ugyanaz a margó
    const qreal leftMargin = 40.0;      // ugyanaz a margó
    const qreal gap = 8.0;              // leftover gap

    qreal y = pageRect.top() + topMargin;

    painter.setRenderHint(QPainter::Antialiasing, false);
    painter.setPen(QPen(Qt::black, 0.75));   // leftover vonalvastagság

    // === 1) Fejléc (pont mint leftover) ===
    painter.drawText(QRectF(leftMargin, y, pageRect.width(), lineH),
                     Qt::AlignLeft,
                     "📝 Anyagfelvételi űrlap");
    y += lineH;

    painter.drawText(QRectF(leftMargin, y, pageRect.width(), lineH),
                     Qt::AlignLeft,
                     QString("📅 Dátum: %1")
                         .arg(QDateTime::currentDateTime().toString("yyyy.MM.dd HH:mm")));
    y += lineH + gap;

    // === 2) Oszlopszélességek (leftover-stílus, teljes szélesség) ===
    qreal totalW = pageRect.width() - leftMargin * 2;

    qreal col1 = totalW * 0.20; // External Code
    qreal col2 = totalW * 0.40; // Material
    qreal col3 = totalW * 0.20; // Qty
    qreal col4 = totalW * 0.20; // Broken

    qreal x1 = leftMargin;
    qreal x2 = leftMargin + col1;
    qreal x3 = leftMargin + col1 + col2;
    qreal x4 = leftMargin + col1 + col2 + col3;

    auto drawRow = [&](qreal yRow,
                       const QString& t1,
                       const QString& t2,
                       const QString& t3,
                       const QString& t4)
    {
        painter.drawText(QRectF(x1 + 5, yRow, col1 - 10, lineH), Qt::AlignLeft, t1);
        painter.drawText(QRectF(x2 + 5, yRow, col2 - 10, lineH), Qt::AlignLeft, t2);
        painter.drawText(QRectF(x3 + 5, yRow, col3 - 10, lineH), Qt::AlignLeft, t3);
        painter.drawText(QRectF(x4 + 5, yRow, col4 - 10, lineH), Qt::AlignLeft, t4);
    };

    auto drawFrame = [&](qreal top, qreal bottom, bool header = false)
    {
        if (header)
            painter.drawLine(leftMargin, top, leftMargin + totalW, top);

        painter.drawLine(leftMargin, bottom, leftMargin + totalW, bottom);

        painter.drawLine(x1, top, x1, bottom);
        painter.drawLine(x2, top, x2, bottom);
        painter.drawLine(x3, top, x3, bottom);
        painter.drawLine(x4, top, x4, bottom);
    };

    // === 3) Két soros leftover-stílusú táblázatfejléc ===
    qreal headerTop = y;
    y += lineH / 2;

    drawRow(y, "External", "Material", "Qty", "Broken");
    y += lineH;

    drawRow(y, "code", "name", "pcs", "pcs");
    y += lineH;

    qreal headerBottom = y;
    drawFrame(headerTop, headerBottom, true);

    //y += lineH / 2;
    lineH = fm.height() * 2;

    // === 4) 15 üres sor leftover-stílusban ===
    for (int i = 0; i < 12; ++i) {
        qreal top = y;
        qreal bottom = y + lineH;

        drawRow(y, "", "", "", "");
        drawFrame(top, bottom);

        y += lineH;
    }
}

} // namespace

