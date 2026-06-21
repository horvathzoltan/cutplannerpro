#pragma once

#include "common/logger.h"
#include "filenamehelper.h"

#include <QFile>
#include <QImage>
#include <QPainter>
#include <QStringList>

namespace EmojiHelper
{

inline QString priorityIconFor_old(int daysLeft)
{
    if (daysLeft<0)
        return "🌞";
    if (daysLeft <= 1)
        return "🔥";   // Tűz – SOS
    if (daysLeft <= 3)
        return "💧";//💦";   // Víz – sürgős 💧 🌊 💦
    if (daysLeft <= 6)
        return "💨";  // Levegő – normál 🌥️  💨
    return "🌱";//⏳";       // Föld – ráér
}

inline QString priorityIconFor(int daysLeft)
{
    if (daysLeft < 0)
        return "earth";   // 🌞 helyett earth.png
    if (daysLeft <= 1)
        return "fire";    // 🔥
    if (daysLeft <= 3)
        return "water";   // 💧
    if (daysLeft <= 6)
        return "wind";    // 💨
    return "earth";       // 🌱 → akár külön PNG is lehet
}


static const QStringList GROUP_ICONS = {
    "🍎", // A - Alma
    "🐸", // B - Béka
    "🐈‍", // C - Macska / 🐈‍
    "🦇", // D - Denevér / 🐝
    "🐭", // E - Egér
    "🍦", // F - Fagyi
    "🦎", // G - Gekkó
    "🌶️", // H - Chili
    "🐛", // I - Kukac
    "🐆", // J - Jaguár
    "❤", // K - szív
    "🦋", // L - Lepke
    "🐿️", // M - Mókus
    "🐰", // N - Nyúl
    "🦁", // O - Oroszlán
    "🐼", // P - Panda
    "🦊", // R - Róka
    "🦔", // S - Süni
    "🐢", // T - Teknős
    "🍇", // V - Szőlő
    "🦓"  // Z - Zebra
};

inline QString getGroupIcon(int groupIndex){
    return GROUP_ICONS[groupIndex % EmojiHelper::GROUP_ICONS.size()];
}

inline QStringList allEmojis(){
    QStringList e;

    e << priorityIconFor(-1);
    e << priorityIconFor(1);
    e << priorityIconFor(3);
    e << priorityIconFor(6);
    e << priorityIconFor(10);

    e << GROUP_ICONS;
    e.removeDuplicates();
    return e;
}

inline int supersampleFactorFor(int targetPx)
{
    if (targetPx <= 32) return 4;
    if (targetPx <= 64) return 3;
    return 2;
}

inline QImage renderEmojiStandalone(const QString& emoji, int size)
{
    if (emoji.isEmpty() || size <= 0)
        return QImage();

    // 1) Supersampling méret
    const int scale = supersampleFactorFor(size);
    const QSize bigSize(size * scale, size * scale);

    QImage img(bigSize, QImage::Format_ARGB32_Premultiplied);
    img.fill(Qt::transparent);

    QPainter p(&img);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setRenderHint(QPainter::TextAntialiasing, true);

    // 2) Font beállítás (0.85× magasság)
    QFont f("Noto Color Emoji");
    f.setPixelSize(static_cast<int>(bigSize.height() * 0.85));
    p.setFont(f);

    // 3) Baseline‑pontos igazítás
    QFontMetrics fm(f);
    const int ascent  = fm.ascent();
    const int descent = fm.descent();
    const int baseline =
        (bigSize.height() + ascent - descent) / 2;

    QRect textRect(0,
                   baseline - ascent,
                   bigSize.width(),
                   fm.height());

    p.drawText(textRect, Qt::AlignHCenter, emoji);
    p.end();

    // 4) Tight bounding box + 1px padding
    QImage src = img.convertToFormat(QImage::Format_ARGB32);
    int minX = bigSize.width();
    int minY = bigSize.height();
    int maxX = -1;
    int maxY = -1;

    for (int y = 0; y < src.height(); ++y) {
        const QRgb* line = reinterpret_cast<const QRgb*>(src.scanLine(y));
        for (int x = 0; x < src.width(); ++x) {
            const QColor c = QColor::fromRgba(line[x]);
            if (c.alpha() == 0)
                continue;

            minX = qMin(minX, x);
            minY = qMin(minY, y);
            maxX = qMax(maxX, x);
            maxY = qMax(maxY, y);
        }
    }

    if (minX > maxX || minY > maxY) {
        // semmi látható pixel
        QImage empty(size, size, QImage::Format_ARGB32_Premultiplied);
        empty.fill(Qt::transparent);
        return empty;
    }

    // 1px padding
    minX = qMax(0, minX - 1);
    minY = qMax(0, minY - 1);
    maxX = qMin(src.width()  - 1, maxX + 1);
    maxY = qMin(src.height() - 1, maxY + 1);

    const QRect tight(minX,
                      minY,
                      maxX - minX + 1,
                      maxY - minY + 1);

    QImage cropped = src.copy(tight);

    // 5) Visszaskálázás Lanczos‑szerű smooth filterrel
    QImage scaled = cropped.scaled(
                               QSize(size, size),
                               Qt::KeepAspectRatio,
                               Qt::SmoothTransformation
                               ).convertToFormat(QImage::Format_ARGB32_Premultiplied);

    zInfo(L("rendered (SSx%1): %2, %3x%3")
              .arg(scale)
              .arg(emoji)
              .arg(size));

    return scaled;
}

inline QString emojiToFileKey(const QString& emoji)
{
    QStringList parts;

    for (int i = 0; i < emoji.size(); ) {
        uint code = emoji.at(i).unicode();

        // Surrogate pair?
        if (0xD800 <= code && code <= 0xDBFF && i + 1 < emoji.size()) {
            uint low = emoji.at(i+1).unicode();
            if (0xDC00 <= low && low <= 0xDFFF) {
                uint full = 0x10000 + ((code - 0xD800) << 10) + (low - 0xDC00);
                parts << QString::number(full, 16);
                i += 2;
                continue;
            }
        }

        // Single code unit
        parts << QString::number(code, 16);
        i += 1;
    }

    return parts.join("_");
}

inline void saveEmojiPng(const QString& emoji, int size)
{
    QString dir = FileNameHelper::emojiCacheDir();

    QString key = emojiToFileKey(emoji);   // <-- FIX
    QString file = QString("%1/emoji_u%2_%3.png")
                       .arg(dir)
                       .arg(key)
                       .arg(size);

    QImage a = renderEmojiStandalone(emoji, size);
    QPixmap px = QPixmap::fromImage(a);
    px.save(file, "PNG");

    zInfo(L("Saved emoji:") + emoji + "→" + file);
}

inline void RenderAllEmojisToCache()
{
    QStringList emojis = allEmojis();
    QList<int> sizes = {32, 64, 96, 128};

    for (const QString& e : emojis) {
        for (int s : sizes) {
            saveEmojiPng(e, s);
        }
    }

    zInfo(L("All emojis rendered to cache."));
}

inline QPixmap loadEmoji(const QString& emoji, int size)
{
    QString key = emojiToFileKey(emoji);

    // 1) Resource pack (:/emojis/...)
    QString resPath = QString(":/emojis/%1/emoji_%2_%1.png")
                          .arg(size)
                          .arg(key);
    if (QFile::exists(resPath))
        return QPixmap(resPath);

    // 2) Cache (~/.CutPlannerPro/emoji_cache)
    QString cachePath = FileNameHelper::emojiCacheDir() +
                        QString("/emoji_%1_%2.png").arg(key).arg(size);

    if (QFile::exists(cachePath))
        return QPixmap(cachePath);

    // 3) Render + cache
    QImage img = renderEmojiStandalone(emoji, size);
    img.save(cachePath, "PNG");
    return QPixmap::fromImage(img);
}

inline QPixmap loadPriorityIcon(const QString& key, int size)
{
    QString path = QString(":/emojis/%1.png").arg(key);

    QPixmap px(path);
    if (!px.isNull())
        return px.scaled(size, size, Qt::KeepAspectRatio, Qt::SmoothTransformation);

    // fallback: ha nincs PNG, akkor emoji karakter
    return EmojiHelper::loadEmoji(key, size);
}


} // emojihelper