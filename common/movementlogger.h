// movementlogger.h
#pragma once

#include "movementlogmodel.h"
#include "filenamehelper.h"
#include <QFile>
#include <QTextStream>
#include <QDate>
#include <QDir>
#include <QFileInfo>

namespace MovementLogger {

inline void log(const MovementLogModel& model) {
    QString filePath = FileNameHelper::instance()
    .getMovementLogFilePathForDate(QDate::currentDate());

    QDir dir(QFileInfo(filePath).absolutePath());
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    QFile file(filePath);
    if (file.open(QIODevice::Append | QIODevice::Text)) {
        QTextStream out(&file);
        out << model.toCsvString() << "\n";
    }
}

} // namespace MovementLogger
