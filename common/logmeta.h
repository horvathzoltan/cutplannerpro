#pragma once

#include <QDateTime>



struct LogMeta {
    QDateTime timestamp;
    QString appVersion;
    QString user;
    QString host;

    QString toString() const {
        return QString("[%1] %2 %3@%4")
        .arg(timestamp.toString("yyyy-MM-dd HH:mm:ss"))
            .arg(appVersion)
            .arg(user)
            .arg(host);
    }
};
