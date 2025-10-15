#include "eventlogger.h"
#include <QDebug>


EventLogger& EventLogger::instance() {
    static EventLogger inst;
    return inst;
}

void EventLogger::setLogFile(const QString& path) {
    file.setFileName(path);
    if (!file.open(QIODevice::Append | QIODevice::Text)) {
        qWarning() << "⚠️ Nem sikerült megnyitni az event log fájlt:" << path;
    } else{
        auto a = timestamped("🟢 START");
        writeToFile(a);
    }
}

QString EventLogger::timestamped(const QString& msg) {
    return QString("[%1] %2")
    .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz"))
        .arg(msg);
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

void EventLogger::writeToFile(const QString& line) {
    QFile f(file.fileName());
    if (f.open(QIODevice::Append | QIODevice::Text)) {
        QTextStream out(&f);
        out << line << "\n";
        f.flush();
        f.close();
    }
}


QStringList EventLogger::loadRecentEvents(int maxLines) {
    QStringList lines;

    QFile f(file.fileName()); // új példány, mindig olvasásra
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

    QFile f(file.fileName());
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
        return lines;

    QStringList all;
    QTextStream in(&f);
    while (!in.atEnd()) {
        all << in.readLine();
    }
    f.close();

    std::reverse(all.begin(), all.end());

    bool foundStart = false;
    for (const QString& line : all) {
        if (line.contains("🟢 START")) {
            foundStart = true;
            break;
        }
        lines << line;
        if (lines.size() >= maxLines)
            break;
    }

    std::reverse(lines.begin(), lines.end());
    return lines;
}
