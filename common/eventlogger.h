#pragma once

#include <QString>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <functional>

// Egységes rövidítések
#define zEvent(msg)  EventLogger::instance().zEvent_((msg))
#define zEventINFO(msg)  EventLogger::instance().zEvent_(EventLogger::Info,  (msg))
#define zEventWARN(msg)  EventLogger::instance().zEvent_(EventLogger::Warning, (msg))
#define zEventERROR(msg) EventLogger::instance().zEvent_(EventLogger::Error, (msg))

class EventLogger {
public:
    enum Level{ Info,Warning,Error };
    static EventLogger& instance();

    void setLogFile(const QString& path);
    void zEvent_(const QString& msg);
    void zEvent_(Level level, const QString& msg);

    // UI callback: pl. QListWidget vagy QPlainTextEdit frissítéséhez
    std::function<void(const QString&)> emitEvent = [](const QString&) {};

    QStringList loadRecentEvents(int maxLines = 50);


    QStringList loadRecentEventsFromLastStart(int maxLines = 50);
private:
    EventLogger() = default;
    //QFile file;
    QString fileName;
    bool writeToFile(const QString &line);
    QString timestamped(const QString &msg);
    bool _isVerbose = false;

    static QString toString(Level level);   // <-- új helper
};
