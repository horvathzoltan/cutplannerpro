#pragma once
#include <QPlainTextEdit>
#include <QStringList>
#include <QTextCursor>

namespace EventLogHelpers {


inline QString ensureNewline(const QString& s) {
    return s.endsWith('\n') ? s : s + '\n';
}


inline void appendLine(QPlainTextEdit* widget, const QString& line) {
    if (!widget) return;

    widget->appendPlainText(ensureNewline(line));

    widget->moveCursor(QTextCursor::End);
    widget->ensureCursorVisible();
}

inline void appendLines(QPlainTextEdit* widget, const QStringList& lines) {
    if (!widget) return;
    if (lines.isEmpty()) return;

    auto txt = lines.join("\n");
    appendLine(widget, txt);
}

inline QColor colorForLine(QPlainTextEdit* widget, const QString& line) {
    if (line.startsWith("ERROR", Qt::CaseInsensitive)) return Qt::red;
    if (line.startsWith("WARN", Qt::CaseInsensitive)) return QColor("#d98e00");
    if (line.startsWith("INFO", Qt::CaseInsensitive)) return Qt::darkGreen;
    return widget->palette().color(QPalette::Text);
}

inline void appendColoredLine(QPlainTextEdit* widget, const QString& line) {
    if (!widget) return;

    QTextCursor cursor(widget->document());
    cursor.movePosition(QTextCursor::End);

    QTextCharFormat fmt;
    fmt.setForeground(colorForLine(widget, line));
    cursor.insertText(ensureNewline(line), fmt);

    widget->ensureCursorVisible();
}

inline void appendColoredLineWithTimestamp(QPlainTextEdit* widget, const QString& line) {
    if (!widget) return;

    QTextCursor cursor(widget->document());
    cursor.movePosition(QTextCursor::End);

    // timestamp leválasztása
    int idx = line.indexOf("] ");
    if (idx != -1) {
        QString ts = line.left(idx + 1) + " ";
        QString payload = line.mid(idx + 2);

        // timestamp mindig alap színnel
        QTextCharFormat tsFmt;
        tsFmt.setForeground(widget->palette().color(QPalette::Text));
        cursor.insertText(ts, tsFmt);

        // prefix + üzenet szétválasztása
        QString prefix;
        QString message = payload;
        int spaceIdx = payload.indexOf(": ");
        if (spaceIdx != -1) {
            prefix = payload.left(spaceIdx + 1);   // pl. "ERROR:"
            message = payload.mid(spaceIdx + 2);   // pl. "adatbázis hiba"
        }

        // prefix mindig fehérrel
        QTextCharFormat prefixFmt;
        prefixFmt.setForeground(Qt::white);
        cursor.insertText(prefix + " ", prefixFmt);

        // üzenet színezve
        QTextCharFormat msgFmt;
        msgFmt.setForeground(colorForLine(widget, payload));
        cursor.insertText(ensureNewline(message), msgFmt);
    } else {
        // fallback: ha nincs timestamp, akkor egész sor egyben
        QTextCharFormat fmt;
        fmt.setForeground(colorForLine(widget, line));
        cursor.insertText(ensureNewline(line), fmt);
    }

    widget->ensureCursorVisible();
}



} // namespace EventLogHelpers
