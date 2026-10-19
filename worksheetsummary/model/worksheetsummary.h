#pragma once

#include "worksheetsummary/model/worksheetorderinfo.h"
#include <QString>
#include <QByteArray>

enum class WorkSheetPresence
{
    Present,
    Missing,
    Unknown
};

namespace WorkSheetPresenceUtils{

inline QString toDisplayText(WorkSheetPresence s){
    switch(s){
    case WorkSheetPresence::Present: return "Present";
    case WorkSheetPresence::Missing: return "Missing";
    case WorkSheetPresence::Unknown: return "Unknown";
    };
    return "Unknown";
}

inline QString toCsv(WorkSheetPresence s) {
    switch (s) {
    case WorkSheetPresence::Present: return "Present";
    case WorkSheetPresence::Missing: return "Missing";
    case WorkSheetPresence::Unknown: return "Unknown";
    }

    return "Unknown";
}

inline WorkSheetPresence fromCsv(const QString& s) {
    if (s.compare("Present", Qt::CaseInsensitive) == 0) return WorkSheetPresence::Present;
    if (s.compare("Missing", Qt::CaseInsensitive) == 0) return WorkSheetPresence::Missing;
    if (s.compare("Unknown", Qt::CaseInsensitive) == 0) return WorkSheetPresence::Unknown;
    return WorkSheetPresence::Unknown;
}

}

struct WorkSheetSummary
{
    QString filePath;
    QString fileName;

    QByteArray fileHash;

    WorkSheetPresence presence = WorkSheetPresence::Present;

    QVector<WorkSheetOrderInfo> orders;

    int requestCount() const
    {
        int sum = 0;

        for (const auto& o : orders)
            sum += o.requestCount;

        return sum;
    }

    int completedCount() const
    {
        int sum = 0;

        for (const auto& o : orders)
            sum += o.completedCount;

        return sum;
    }

    int uncompletedCount() const
    {
        int sum = 0;

        for (const auto& o : orders)
            sum += o.uncompletedCount;

        return sum;
    }

    int undefinedCount() const
    {
        int sum = 0;

        for (const auto& o : orders)
            sum += o.undefinedCount;

        return sum;
    }

    bool requiresAttention() const
    {
        return
            undefinedCount() > 0 ||
            uncompletedCount() > 0;
    }
};