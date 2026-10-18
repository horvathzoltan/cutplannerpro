#pragma once

#include <QString>
#include <QByteArray>

enum class WorkSheetPresence
{
    Present,
    Missing
};

struct WorkSheetSummary
{
    QString filePath;
    QString fileName;

    QByteArray fileHash;

    int requestCount = 0;

    int completedCount = 0;

    int uncompletedCount = 0;

    int undefinedCount = 0;

    WorkSheetPresence presence = WorkSheetPresence::Present;
;

    bool requiresAttention() const
    {
        return
            undefinedCount > 0 ||
            uncompletedCount > 0;
    }
};