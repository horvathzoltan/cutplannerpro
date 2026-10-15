#pragma once

#include "worksheetsummary/model/worksheetsummary.h"

#include <QVector>
#include <QString>

class WorkSheetSummaryRegistry
{
public:
    static WorkSheetSummaryRegistry& instance();

    void clear();

    bool registerData(const WorkSheetSummary& summary);

    const QVector<WorkSheetSummary>& readAll() const;

    WorkSheetSummary* findByFilePath(
        const QString& filePath);

    const WorkSheetSummary* findByFilePath(
        const QString& filePath) const;

    bool isEmpty() const
    {
        return _data.isEmpty();
    }

    int size() const
    {
        return _data.size();
    }

private:
    WorkSheetSummaryRegistry() = default;

private:
    QVector<WorkSheetSummary> _data;
};