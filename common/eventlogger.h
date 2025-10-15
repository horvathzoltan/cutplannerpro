#pragma once

#include <QString>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <functional>

class EventLogger {
public:
    static EventLogger& instance();

    void setLogFile(const QString& path);
    void zEvent(const QString& msg);

    // UI callback: pl. QListWidget vagy QPlainTextEdit frissítéséhez
    std::function<void(const QString&)> emitEvent = [](const QString&) {};

    QStringList loadRecentEvents(int maxLines = 50);


    QStringList loadRecentEventsFromLastStart(int maxLines = 50);
private:
    EventLogger() = default;
    QFile file;
    void writeToFile(const QString &line);
    QString timestamped(const QString &msg);
    bool _isVerbose = false;
};
