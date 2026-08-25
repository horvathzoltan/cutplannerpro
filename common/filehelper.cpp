#include "filehelper.h"
#include "logger.h"

#include <QFile>
#include <QMap>

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

FileHelper::SeparatorResult FileHelper::detectSeparatorSmart(QTextStream* st) {
    SeparatorResult result;

    QList<QChar> candidates = { ',', ';', '\t', '|' };

    QStringList lines;
    while (!st->atEnd() && lines.size() < 2) {
        QString line = st->readLine().trimmed();
        if (line.isEmpty()) continue;
        if (line.startsWith('#')) continue;   // ⬅️ komment átugrása
        lines.append(line);
    }

    if (lines.size() < 2) {
        result.globalWarnings << "Nincs elég sor";
        result.hasError = true;
        result.isSingleColumn = false;
        return result; // ❌ Nem elég sor
    }

    result.isSingleColumn = true;

    for (const QChar& sep : candidates) {
        QStringList localWarnings;

        QTextStream testStream(lines.join("\n").toUtf8());
        QList<QVector<QString>> rows = FileHelper::parseCSV(&testStream, sep);

        if (rows.size() < 2)
            continue; // vagy return QChar();

        if (rows[0].isEmpty() || rows[1].isEmpty())
            continue;

        // int headerFieldCount = std::count_if(rows[0].begin(), rows[0].end(), [](const QString& s) {
        //     return !s.trimmed().isEmpty();
        // });

        // --- HEADER FIELD COUNT: last non-empty header field ---

        auto header = rows[0];
        int lastNamedIndex = -1;
        int headerFieldCount = 0;
        bool hasHole = false;

        for (int i = 0; i < header.size(); ++i) {

            bool named = !header[i].trimmed().isEmpty();

            if (named) {
                // frissítjük az utolsó nevesített mező indexét
                lastNamedIndex = i;
                headerFieldCount = i + 1;   // ← itt számoljuk
            } else {
                // ha üres mezőt találunk a nevesített mezők között → lyuk
                if (i < lastNamedIndex) {
                    hasHole = true;
                }
            }
        }

        if(hasHole){
            localWarnings << "❌ A fejléc tartalmaz lyukakat";
            result.hasError = true;
        }

        if (lastNamedIndex < 0){
            localWarnings <<"❌ Nincs egyetlen nevesített mező sem";
            result.hasError = true;
        }

        // --- DATA FIELD COUNT ---
        int dataFieldCount = rows[1].size();

        if (dataFieldCount > headerFieldCount){
            localWarnings << L("⚠️ Extra mezők az adat sor végén (%1 extra).")
                                 .arg(dataFieldCount - headerFieldCount);
            result.hasWarning = true;
        }

        bool ok = !hasHole &&
                  lastNamedIndex >=0 &&
                  headerFieldCount >= 2 &&
                  dataFieldCount >= 2 &&
                  dataFieldCount >= headerFieldCount;

        if(!localWarnings.isEmpty())
            result.separatorWarnings.insert(sep, localWarnings);

        // ha bármelyik szeparátorral több mező van → nem singleColumn
        if (headerFieldCount > 1 || dataFieldCount > 1)
            result.isSingleColumn = false;

        if(ok){
            //result.hasError = false;
            result.separator = sep;
            result.isSingleColumn = false;
            return result; // 🎯 Találtunk jó szeparátort
        }
    }

    if (result.separator.isNull()) {
        if (result.isSingleColumn) {
            // valódi egyoszlopos CSV
            //result.hasError = false;
            return result;
        } else {
            // hibás CSV
            result.hasError = true;
            result.globalWarnings << "❌ Nem sikerült szeparátort detektálni.";
            return result;
        }
    }

    result.separator = QChar();
    result.isSingleColumn = false;
    result.hasError = true;
    result.globalWarnings << "❌ Nem sikerült szeparátort detektálni a fejléc alapján.";
    return result;

}
