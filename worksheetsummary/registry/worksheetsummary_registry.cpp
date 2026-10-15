#include "worksheetsummary_registry.h"

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