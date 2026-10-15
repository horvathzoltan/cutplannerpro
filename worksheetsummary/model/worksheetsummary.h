#pragma once

#include <QString>
#include <QByteArray>

struct WorkSheetSummary
{
    QString filePath;
    QString fileName;

    QByteArray fileHash;

    int requestCount = 0;

    int completedCount = 0;

    int uncompletedCount = 0;

    int undefinedCount = 0;

    bool requiresAttention() const
    {
        return
            undefinedCount > 0 ||
            uncompletedCount > 0;
    }
};