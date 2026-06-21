#pragma once

#include <QStandardPaths>
#include <QFile>
#include <QFileInfo>
#include <QByteArray>
#include <QString>
#include <QFontDatabase>
#include <QDir>
#include <QRegularExpression>

class FontVersionHelper
{
public:

    static QStringList listFontFilesRecursively(const QString& root)
    {
        QStringList result;
        QDir dir(root);

        QFileInfoList entries = dir.entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);

        for (const QFileInfo& fi : entries) {
            if (fi.isDir()) {
                result += listFontFilesRecursively(fi.absoluteFilePath());
            } else {
                if (fi.suffix().toLower() == "ttf" || fi.suffix().toLower() == "otf")
                    result << fi.absoluteFilePath();
            }
        }
        return result;
    }

    static QStringList fontRoots()
    {
        QStringList roots;

        // Qt által ismert
        roots << QStandardPaths::standardLocations(QStandardPaths::FontsLocation);

        // Linux fontconfig standard könyvtárak
        roots << "/usr/share/fonts";
        roots << "/usr/share/fonts/truetype";
        roots << "/usr/share/fonts/opentype";
        roots << "/usr/local/share/fonts";

        // User fonts
        roots << QDir::homePath() + "/.local/share/fonts";
        roots << QDir::homePath() + "/.fonts";

        // Duplikátumok kiszűrése
        roots.removeDuplicates();

        return roots;
    }

    static QString findFontFileForFamily(const QString& family)
    {
        QStringList roots = fontRoots();

        for (const QString& root : roots) {
            QStringList files = listFontFilesRecursively(root);

            for (const QString& path : files) {
                int id = QFontDatabase::addApplicationFont(path);
                if (id < 0)
                    continue;

                QStringList fams = QFontDatabase::applicationFontFamilies(id);
                QFontDatabase::removeApplicationFont(id);

                if (fams.contains(family))
                    return path;
            }
        }

        return "";
    }



    static QString getFontVersion(const QString& fullPath)
    {
        QFile f(fullPath);
        if (!f.open(QIODevice::ReadOnly))
            return "Open error";

        QByteArray data = f.readAll();
        f.close();

        if (data.size() < 12)
            return "Too small";

        const uchar* buf = reinterpret_cast<const uchar*>(data.constData());

        auto u16 = [&](int offset) -> quint16 {
            return (quint16(buf[offset]) << 8) | quint16(buf[offset + 1]);
        };

        auto u32 = [&](int offset) -> quint32 {
            return (quint32(buf[offset]) << 24) |
                   (quint32(buf[offset + 1]) << 16) |
                   (quint32(buf[offset + 2]) << 8) |
                   quint32(buf[offset + 3]);
        };

        quint16 numTables = u16(4);
        if (data.size() < 12 + numTables * 16)
            return "Invalid table directory";

        int nameTableOffset = -1;

        for (int i = 0; i < numTables; ++i) {
            int entryOffset = 12 + i * 16;
            quint32 tag = u32(entryOffset);

            if (tag == 0x6E616D65) { // 'name'
                nameTableOffset = u32(entryOffset + 8);
                break;
            }
        }

        if (nameTableOffset < 0 || nameTableOffset + 6 > data.size())
            return "No name table";

        quint16 count = u16(nameTableOffset + 2);
        quint16 stringOffset = u16(nameTableOffset + 4);

        int nameRecordBase = nameTableOffset + 6;
        int stringStorageBase = nameTableOffset + stringOffset;

        for (int i = 0; i < count; ++i) {
            int recOffset = nameRecordBase + i * 12;

            quint16 platformID = u16(recOffset + 0);
            quint16 nameID     = u16(recOffset + 6);
            quint16 length     = u16(recOffset + 8);
            quint16 offset     = u16(recOffset + 10);

            if (nameID != 5)
                continue;

            int strPos = stringStorageBase + offset;
            if (strPos + length > data.size())
                continue;

            QByteArray raw = data.mid(strPos, length);

            if (platformID == 3) { // Windows UTF‑16BE
                if (raw.size() % 2 != 0)
                    continue;

                QString s;
                for (int j = 0; j < raw.size(); j += 2) {
                    quint16 ch = (quint16(quint8(raw[j])) << 8) |
                                 quint16(quint8(raw[j + 1]));
                    s.append(QChar(ch));
                }
                return s;
            }

            return QString::fromLatin1(raw);
        }

        return "Unknown";
    }

    struct FontVersionInfo
    {
        QString raw;          // teljes verzió string
        QString version;      // "Version 2.047"
        int major = 0;
        int minor = 0;

        QString vendor;       // "GOOG" vagy üres
        QString buildDate;    // "20240827" vagy üres
        QString commitHash;   // git hash vagy üres
        QString hintingInfo;  // ttfautohint paraméterek vagy üres
    };

    static FontVersionInfo parseVersionString(const QString& raw)
    {
        FontVersionInfo info;
        info.raw = raw.trimmed();

        if (raw.isEmpty())
            return info;

        // 1) Split by semicolon
        QStringList parts = raw.split(';', Qt::SkipEmptyParts);

        // --- Version (always first) ---
        info.version = parts.value(0).trimmed();

        // Extract major.minor
        QRegularExpression re("Version\\s+(\\d+)\\.(\\d+)");
        auto m = re.match(info.version);
        if (m.hasMatch()) {
            info.major = m.captured(1).toInt();
            info.minor = m.captured(2).toInt();
        }

        // --- Vendor (Google fonts) ---
        if (parts.size() > 1) {
            QString p = parts[1].trimmed();
            if (p.size() <= 6 && p.contains(QRegularExpression("^[A-Za-z0-9]+$")))
                info.vendor = p;
        }

        // --- Build date + commit hash (Google Noto Emoji) ---
        if (parts.size() > 2) {
            QString p = parts[2].trimmed();
            QStringList sub = p.split(':');
            if (sub.size() >= 2) {
                // noto-emoji:20240827:hash
                if (sub[0].contains("noto")) {
                    info.buildDate = sub.value(1);
                    if (sub.size() >= 3)
                        info.commitHash = sub.value(2);
                }
            }
        }

        // --- Hinting info (Noto Sans Mono) ---
        if (parts.size() > 1 && parts[1].contains("ttfautohint"))
            info.hintingInfo = parts[1].trimmed();

        return info;
    }


};
