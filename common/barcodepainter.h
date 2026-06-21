#pragma once
#include <QPainter>
#include <QString>

namespace BarcodePainter
{
// Code128 karakterkészlet (A+B+C)
static const int CODE128_PATTERNS[106][6] = {
    {2,1,2,2,2,2}, {2,2,2,1,2,2}, {2,2,2,2,2,1}, {1,2,1,2,2,3},
    {1,2,1,3,2,2}, {1,3,1,2,2,2}, {1,2,2,2,1,3}, {1,2,2,3,1,2},
    {1,3,2,2,1,2}, {2,2,1,2,1,3}, {2,2,1,3,1,2}, {2,3,1,2,1,2},
    {1,1,2,2,3,2}, {1,2,2,1,3,2}, {1,2,2,2,3,1}, {1,1,3,2,2,2},
    {1,2,3,1,2,2}, {1,2,3,2,2,1}, {2,2,3,2,1,1}, {2,2,1,1,3,2},
    {2,2,1,2,3,1}, {2,1,3,2,1,2}, {2,2,3,1,1,2}, {3,1,2,1,3,1},
    {3,1,1,2,2,2}, {3,2,1,1,2,2}, {3,2,1,2,2,1}, {3,1,2,2,1,2},
    {3,2,2,1,1,2}, {3,2,2,2,1,1}, {2,1,2,1,2,3}, {2,1,2,3,2,1},
    {2,3,2,1,2,1}, {1,1,1,3,2,3}, {1,3,1,1,2,3}, {1,3,1,3,2,1},
    {1,1,2,3,1,3}, {1,3,2,1,1,3}, {1,3,2,3,1,1}, {2,1,1,3,1,3},
    {2,3,1,1,1,3}, {2,3,1,3,1,1}, {1,1,2,1,3,3}, {1,1,2,3,3,1},
    {1,3,2,1,3,1}, {1,1,3,1,2,3}, {1,1,3,3,2,1}, {1,3,3,1,2,1},
    {3,1,3,1,2,1}, {2,1,1,3,3,1}, {2,3,1,1,3,1}, {2,1,3,1,1,3},
    {2,1,3,3,1,1}, {2,1,3,1,3,1}, {3,1,1,1,2,3}, {3,1,1,3,2,1},
    {3,3,1,1,2,1}, {3,1,2,1,1,3}, {3,1,2,3,1,1}, {3,3,2,1,1,1},
    {3,1,4,1,1,1}, {2,2,1,4,1,1}, {4,3,1,1,1,1}, {1,1,1,2,2,4},
    {1,1,1,4,2,2}, {1,2,1,1,2,4}, {1,2,1,4,2,1}, {1,4,1,1,2,2},
    {1,4,1,2,2,1}, {1,1,2,2,1,4}, {1,1,2,4,1,2}, {1,2,2,1,1,4},
    {1,2,2,4,1,1}, {1,4,2,1,1,2}, {1,4,2,2,1,1}, {2,4,1,2,1,1},
    {2,2,1,1,1,4}, {4,1,3,1,1,1}, {2,4,1,1,1,2}, {1,3,4,1,1,1},
    {1,1,1,2,4,2}, {1,2,1,1,4,2}, {1,2,1,2,4,1}, {1,1,4,2,1,2},
    {1,2,4,1,1,2}, {1,2,4,2,1,1}, {4,1,1,2,1,2}, {4,2,1,1,1,2},
    {4,2,1,2,1,1}, {2,1,2,1,4,1}, {2,1,4,1,2,1}, {4,1,2,1,2,1},
    {1,1,1,1,4,3}, {1,1,1,3,4,1}, {1,3,1,1,4,1}, {1,1,4,1,1,3},
    {1,1,4,3,1,1}, {4,1,1,1,1,3}, {4,1,1,3,1,1}, {1,1,3,1,4,1},
    {1,1,4,1,3,1}, {3,1,1,1,4,1}, {4,1,1,1,3,1}, {2,1,1,4,1,2},
    {2,1,1,2,1,4}, {2,1,1,2,3,2}
};

static const int CODE128_STOP[7] = {2,3,3,1,1,1,2};



inline qreal drawPattern(QPainter& p,
                         const int* pattern,
                         int length,
                         qreal x,
                         const QRectF& rect,
                         qreal barWidth)
{
    for (int i = 0; i < length; ++i) {
        qreal w = pattern[i] * barWidth;
        if (i % 2 == 0) {
            p.fillRect(QRectF(x, rect.top(), w, rect.height()), Qt::black);
        }
        x += w;
    }
    return x;
}


inline void drawCode128(QPainter& p, const QString& text, const QRectF& rect)
{
    // Start Code B
    int checksum = 104;
    QVector<int> codes;
    codes.append(104);

    for (int i = 0; i < text.size(); ++i) {
        int c = text[i].unicode();
        if (c >= 32 && c <= 126) {
            int code = c - 32;
            codes.append(code);
            checksum += code * (i + 1);
        }
    }

    checksum %= 103;
    codes.append(checksum);

    // STOP nem kerül a codes-be, külön rajzoljuk

    qreal x = rect.left();

    // teljes modulok száma = normál kódok * 11 + STOP * 13
    qreal totalModules = codes.size() * 11 + 13;
    qreal barWidth = rect.width() / totalModules;

    // Normál kódok (0–105)
    for (int code : codes) {
        x = drawPattern(p,
                        CODE128_PATTERNS[code],
                        6,
                        x,
                        rect,
                        barWidth);
    }

    // STOP (106)
    x = drawPattern(p,
                    CODE128_STOP,
                    7,
                    x,
                    rect,
                    barWidth);
}





} // end of namespace
