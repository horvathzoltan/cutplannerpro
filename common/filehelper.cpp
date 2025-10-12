#include "filehelper.h"
#include "common/logger.h"

#include <QFile>

// Megjegyzés: a parser automatikusan kihagyja az üres sorokat a fájl feldolgozása során.
/*
QList<QVector<QString>> FileHelper::parseCSV(QTextStream *st, const QChar& separator)
{
    QList<QVector<QString>> rows;
    QVector<QString> fields;
    QString s;
    bool inQuote = false;

    while (!st->atEnd()) {
        QString line = st->readLine();

        // Üres sorok kihagyása
        if (line.trimmed().isEmpty()) continue;

        // Többsoros idézőzött cellák beolvasása
        while (inQuote && !st->atEnd()) {
            line += "\n" + st->readLine();
        }

        fields.clear();
        s.clear();
        inQuote = false;

        for (int i = 0; i < line.size(); ++i) {
            QChar ch = line[i];

            if (ch == '"') {
                if (!inQuote) {
                    inQuote = true;
                } else {
                    if (i + 1 < line.size() && line[i + 1] == '"') {
                        s += '"'; ++i; // Escaped idézőjel: ""
                    } else {
                        inQuote = false; // Lezárás
                    }
                }
                continue;
            }

            if (!inQuote && ch == separator) {
                fields.append(parseCell(s)); // Cellák feldolgozása
                s.clear();
            } else {
                // Escape karakter kezelése idézőn kívül
                if (ch == '\\' && i + 1 < line.size()) {
                    QChar next = line[i + 1];
                    switch (next.unicode()) {
                    case 'n': s += '\n'; break;
                    case 't': s += '\t'; break;
                    case '\\': s += '\\'; break;
                    case '"': s += '"'; break;
                    default: s += ch;
                    }
                    ++i;
                } else {
                    s += ch;
                }
            }
        }

        fields.append(parseCell(s)); // Utolsó cella hozzáadása
        rows.append(fields);
    }

    return rows;
}*/

QList<QVector<QString>> FileHelper::parseCSV(QTextStream *st, const QChar& separator)
{
    QList<QVector<QString>> rows;
    if (!st) return rows;

    QString partialLine;
    while (!st->atEnd()) {
        QString line = st->readLine();

        // Üres sorok kihagyása
        if (partialLine.isEmpty() && line.trimmed().isEmpty()) continue;

        // Accumulate lines until quotes are balanced (handles multiline quoted cells)
        if (partialLine.isEmpty()) partialLine = line;
        else partialLine += '\n' + line;

        // Count quote characters, but ignore escaped double quotes ""
        int quoteCount = 0;
        for (int i = 0; i < partialLine.size(); ++i) {
            if (partialLine[i] == '"') {
                // if next char is also '"', skip the pair as escaped quote
                if (i + 1 < partialLine.size() && partialLine[i + 1] == '"') {
                    ++i; // skip escaped quote pair
                    continue;
                }
                ++quoteCount;
            }
        }

        // If odd number of quotes -> still inside quoted field, read next line
        if ((quoteCount & 1) != 0) {
            // continue reading more lines to complete the quoted field
            continue;
        }

        // Now partialLine contains a full logical CSV line; parse fields
        QVector<QString> fields;
        QString cell;
        bool inQuote = false;

        for (int i = 0; i < partialLine.size(); ++i) {
            QChar ch = partialLine[i];

            if (ch == '"') {
                if (!inQuote) {
                    inQuote = true;
                    // if quote is immediately followed by another quote, it's an escaped quote start,
                    // but we'll handle escaped quotes in the inQuote branch below
                    continue;
                } else {
                    // if next char is also a quote -> escaped quote
                    if (i + 1 < partialLine.size() && partialLine[i + 1] == '"') {
                        cell += '"';
                        ++i;
                        continue;
                    } else {
                        inQuote = false;
                        continue;
                    }
                }
            }

            if (!inQuote && ch == separator) {
                fields.append(parseCell(cell));
                cell.clear();
            } else {
                // handle backslash escapes outside quotes as before
                if (!inQuote && ch == '\\' && i + 1 < partialLine.size()) {
                    QChar next = partialLine[i + 1];
                    switch (next.unicode()) {
                    case 'n': cell += '\n'; break;
                    case 't': cell += '\t'; break;
                    case '\\': cell += '\\'; break;
                    case '"': cell += '"'; break;
                    default: cell += ch;
                    }
                    ++i;
                } else {
                    cell += ch;
                }
            }
        }

        fields.append(parseCell(cell));
        rows.append(fields);

        // reset for next logical line
        partialLine.clear();
    }

    return rows;
}

QString FileHelper::parseCell(const QString& rawCell) {
    QString result;
    bool inEscape = false;

    for (int i = 0; i < rawCell.size(); ++i) {
        QChar ch = rawCell[i];

        if (inEscape) {
            // Escape karakterek értelmezése
            switch (ch.unicode()) {
            case 'n': result += '\n'; break;
            case 't': result += '\t'; break;
            case '"': result += '"'; break;
            case '\\': result += '\\'; break;
            default: result += ch;
            }
            inEscape = false;
        } else {
            if (ch == '\\') {
                inEscape = true;
            } else {
                result += ch;
            }
        }
    }

    return result.trimmed(); // Felesleges whitespace-ek eltávolítása
}

bool FileHelper::isCsvWithOnlyHeader(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return false;

    QTextStream in(&file);
    in.setEncoding(QStringConverter::Utf8);

    int lineCount = 0;
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (!line.isEmpty()) lineCount++;
        if (lineCount > 1) break;
    }

    return lineCount == 1; // csak a fejléc
}

QChar FileHelper::detectSeparatorSmart(QTextStream* st) {
    QList<QChar> candidates = { ',', ';', '\t', '|' };

    QStringList lines;
    while (!st->atEnd() && lines.size() < 2) {
        QString line = st->readLine().trimmed();
        if (!line.isEmpty()) lines.append(line);
    }

    if (lines.size() < 2) return QChar(); // ❌ Nem elég sor

    for (const QChar& sep : candidates) {
        QTextStream testStream(lines.join("\n").toUtf8());
        QList<QVector<QString>> rows = FileHelper::parseCSV(&testStream, sep);

        int headerFieldCount = std::count_if(rows[0].begin(), rows[0].end(), [](const QString& s) {
            return !s.trimmed().isEmpty();
        });

        int dataFieldCount = rows[1].size();

        bool ok = headerFieldCount >= 2 &&
                  dataFieldCount >= 2 &&
                  dataFieldCount == headerFieldCount;

        // bool ok = rows.size() >= 2 &&
        //           rows[0].size() == rows[1].size() &&
        //           rows[0].size() >= 2; // 🔍 legalább 2 mező legyen
        if (ok) {
            //QString msg = QStringLiteral("✅ Szeparátor detektálva:%1 -> mezők:%2").arg(sep).arg(rows[0].size());
            //zInfo(msg);
            return sep; // 🎯 Találtunk jó szeparátort
        }
    }

    zWarning("❌ Nem sikerült szeparátort detektálni a fejléc alapján.");
    return QChar(); // ❌ Nem sikerült detektálni
}
