#include "eventlogger.h"
#include <QDebug>


EventLogger& EventLogger::instance() {
    static EventLogger inst;
    return inst;
}

void EventLogger::setLogFile(const QString& path) {
    fileName = path;

    auto a = timestamped("🟢 START");

    if (!writeToFile(a)) {
        qWarning() << "⚠️ Nem sikerült megnyitni az event log fájlt:" << path;
    }
}

QString EventLogger::timestamped(const QString& msg) {
    QStringList lines = msg.split("\n");
    QString ts = QDateTime::currentDateTime().toString(Qt::ISODateWithMs);

    if (lines.isEmpty())
        return QString("[%1] ").arg(ts);

    QString clean = lines.first();

    // Láthatatlan whitespace-ek eltávolítása
    static const QVector<QChar> badChars = {
        QChar(0x200B), // ZERO-WIDTH SPACE
        QChar(0xFEFF), // ZERO-WIDTH NO-BREAK SPACE
        QChar(0x2060), // WORD JOINER
        QChar(0x00A0), // NO-BREAK SPACE
        QChar(0x202F)  // NARROW NO-BREAK SPACE
    };
    for (QChar c : badChars)
        clean.remove(c);

    clean = clean.trimmed();

    // ⭐ Emoji padding fix:
    // Ha az első karakter emoji (U+1F300 felett),
    // akkor a timestamp utáni space és az emoji közé beszúrunk egy WORD JOINER-t.
    QString spacer = " ";
    if (!clean.isEmpty() && clean.at(0).unicode() >= 0x1F300) {
        spacer.append(QChar(0x2060));  // WORD JOINER
    }

    QString first = QString("[%1]%2%3").arg(ts).arg(spacer).arg(clean);

    // Többsoros indent
    int indentSize = ts.length() + 2;
    QString indent(indentSize, ' ');
    for (int i = 1; i < lines.size(); ++i)
        lines[i] = indent + lines[i];

    lines[0] = first;


    return lines.join("\n");
}




QString EventLogger::toString(Level level) {
    switch (level) {
    case Info:    return "INFO: ";
    case Warning: return "WARNING: ";
    case Error:   return "ERROR: ";
    }
    return "LOG: "; // fallback, ha bővül az enum
}

void EventLogger::zEvent_(const QString& msg) {
    QString line = timestamped(msg);

    // if (msg.toLower().contains("optim")) {
    //     qInfo("optim");
    // }

    if(_isVerbose)
    {
        qInfo().noquote() << "EVENT:" << line;
    }
    emitEvent(line);
    writeToFile(line);
}

void EventLogger::zEvent_(const QStringList& lines) {
    QString a = lines.join("\n");
    zEvent_(a);
    // if (lines.isEmpty())
    //     return;

    // QString block = lines.join("\n");
    // QString stamped = timestamped(block);

    // emitEvent(stamped);
    // writeToFile(stamped);
}

void EventLogger::zEvent_(Level level, const QString& msg) {
    this->zEvent_(toString(level) + msg);
}


bool EventLogger::writeToFile(const QString& line) {
    if (fileName.isEmpty()) return false;

    QFile file(fileName);
    if (!file.open(QIODevice::Append | QIODevice::Text))
        return false;

    QTextStream out(&file);
    out << line << "\n";
    return true;
}


QStringList EventLogger::loadRecentEvents(int maxLines) {
    QStringList lines;

    QFile f(fileName); // új példány, mindig olvasásra
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
        return lines;

    QTextStream in(&f);
    while (!in.atEnd()) {
        lines << in.readLine();
    }
    f.close();

    std::reverse(lines.begin(), lines.end());
    return lines.mid(0, maxLines);
}

QStringList EventLogger::loadRecentEventsFromLastStart(int maxLines) {
    QStringList lines;

    QFile f(fileName);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
        return lines;

    QStringList all;
    QTextStream in(&f);
    while (!in.atEnd()) {
        all << in.readLine();
    }
    f.close();

    std::reverse(all.begin(), all.end());

    for (const QString& line : all) {
        if (line.contains("🟢 START")) {
            break;
        }

        lines << line;
        if (lines.size() >= maxLines)
            break;
    }

    std::reverse(lines.begin(), lines.end());
    return lines;
}
