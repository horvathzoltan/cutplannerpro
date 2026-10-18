#include "worksheetsummary_registry.h"
#include "common/logger.h"

WorkSheetSummaryRegistry&
WorkSheetSummaryRegistry::instance()
{
    static WorkSheetSummaryRegistry registry;
    return registry;
}

void WorkSheetSummaryRegistry::clear()
{
    _data.clear();
}

bool WorkSheetSummaryRegistry::registerData(
    const WorkSheetSummary& summary)
{
    for (auto& item : _data)
    {
        if (item.filePath == summary.filePath)
        {
            item = summary;
            return false;   // update történt
        }
    }

    _data.push_back(summary);
    return true;            // új elem
}

const QVector<WorkSheetSummary>&
WorkSheetSummaryRegistry::readAll() const
{
    return _data;
}

QVector<WorkSheetSummary>&
WorkSheetSummaryRegistry::readAll()
{
    return _data;
}



WorkSheetSummary*
WorkSheetSummaryRegistry::findByFilePath(
    const QString& filePath)
{
    for (auto& item : _data)
    {
        if (item.filePath == filePath)
            return &item;
    }

    return nullptr;
}

const WorkSheetSummary*
WorkSheetSummaryRegistry::findByFilePath(
    const QString& filePath) const
{
    for (const auto& item : _data)
    {
        if (item.filePath == filePath)
            return &item;
    }

    return nullptr;
}

WorkSheetSummary*
WorkSheetSummaryRegistry::findByFileName(
    const QString& fileName)
{
    for (auto& item : _data)
    {
        if (item.fileName == fileName)
            return &item;
    }

return nullptr;
}



void WorkSheetSummaryRegistry::debugDump() const
{
    zInfo("==============================================");
    zInfo("🔍 WorkSheetSummaryRegistry dump indul");
    zInfo("==============================================");

    if (_data.isEmpty())
    {
        zWarning("⚠️ Nincs egyetlen worksheet summary sem.");
        return;
    }

    for (const auto& item : _data)
    {
        QString presence =
            item.presence ==
                    WorkSheetPresence::Present
                ? "Present"
                : "Missing";

        zInfo(
            QString(
                "📄 %1")
                .arg(item.fileName));

        zInfo(
            QString(
                "   Presence      : %1")
                .arg(presence));

        zInfo(
            QString(
                "   Requests      : %1")
                .arg(item.requestCount));

        zInfo(
            QString(
                "   Completed     : %1")
                .arg(item.completedCount));

        zInfo(
            QString(
                "   Uncompleted   : %1")
                .arg(item.uncompletedCount));

        zInfo(
            QString(
                "   Undefined     : %1")
                .arg(item.undefinedCount));

        zInfo(
            QString(
                "   Attention     : %1")
                .arg(
                    item.requiresAttention()
                        ? "YES"
                        : "NO"));

        zInfo("");
    }

    zInfo("==============================================");
    zInfo("🔚 WorkSheetSummaryRegistry dump vége");
    zInfo("==============================================");
}
