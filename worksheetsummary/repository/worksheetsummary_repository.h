#pragma once

#include "common/csvimporter.h"
#include "worksheetsummary/model/worksheetsummary.h"
#include "worksheetsummary/registry/worksheetsummary_registry.h"

#include <QString>
#include <QVector>
#include <optional>

class WorkSheetSummaryRepository
{
public:

    static bool loadFromFile(
        WorkSheetSummaryRegistry& registry,
        const QString& filePath);

    static bool saveToFile(
        const WorkSheetSummaryRegistry& registry,
        const QString& filePath);

private:

    struct WorkSheetSummaryRow
    {
        QString filePath;
        QString fileName;

        QString fileHash;

        int requestCount = 0;

        int completedCount = 0;
        int uncompletedCount = 0;

        int undefinedCount = 0;
    };

    enum class CSVVersion
    {
        V1,
        Unknown
    };

private:

    static QVector<WorkSheetSummary>
    loadFromCsv_private(
        CsvReader::FileContext& ctx);

    static CSVVersion detectCsvVersion(
        const QString& filePath);

    static std::optional<WorkSheetSummaryRow>
    convertRowToWorkSheetSummaryRow(
        const QVector<QString>& parts,
        CsvReader::FileContext& ctx);

    static std::optional<WorkSheetSummary>
    convertRowToWorkSheetSummary(
        const QVector<QString>& parts,
        CsvReader::FileContext& ctx);
};