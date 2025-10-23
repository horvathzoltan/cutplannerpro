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
    return QString("[%1] %2")
    .arg(QDateTime::currentDateTime().toString(Qt::ISODateWithMs))
        .arg(msg);
}

QString EventLogger::toString(Level level) {
    switch (level) {
    case Info:    return "INFO: ";
    case Warning: return "WARNING: ";
    case Error:   return "ERROR: ";
    }
    return "LOG: "; // fallback, ha bővül az enum
}

void EventLogger::zEvent(const QString& msg) {
    QString line = timestamped(msg);
    if(_isVerbose)
    {
        qInfo().noquote() << "EVENT:" << line;
    }
    emitEvent(line);
    writeToFile(line);
}

void EventLogger::zEvent(Level level, const QString& msg) {
    this->zEvent(toString(level) + msg);
}


bool EventLogger::writeToFile(const QString& line) {
    QFile f(fileName);
    if (f.open(QIODevice::Append | QIODevice::Text)) {
        QTextStream out(&f);
        out << line << "\n";
        f.close();
        return true;
    } else{
        return false;
    }
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
