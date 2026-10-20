#pragma once

#include "common/csvimporter.h"

#include "worksheetsummary/model/worksheetsummary.h"
#include "worksheetsummary/model/worksheetorderinfo.h"
#include "worksheetsummary/registry/worksheetsummary_registry.h"

#include <QString>
#include <QVector>
#include <QList>

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

        QString presence;
    };

    struct WorkSheetOrderInfoRow
    {
        QString externalReference;

        QString ownerName;

        QString dueDate;

        QString productTypeCode;

        QString productSubtypeCode;

        QString requiredColorCode;

        int requestCount = 0;

        int completedCount = 0;

        int uncompletedCount = 0;

        int undefinedCount = 0;
    };

private:

    static QList<QList<QVector<QString>>>
    readMsffSections(
        const QString& filepath);

    static bool parseMsffSections(
        const QList<QList<QVector<QString>>>& sections,
        WorkSheetSummaryRegistry& registry);

private:

    static std::optional<WorkSheetSummaryRow>
    convertRowToWorkSheetSummaryRow(
        const QVector<QString>& parts,
        CsvReader::FileContext& ctx);

    static std::optional<WorkSheetOrderInfoRow>
    convertRowToWorkSheetOrderInfoRow(
        const QVector<QString>& parts,
        CsvReader::FileContext& ctx);

private:

    static std::optional<WorkSheetSummary>
    buildWorkSheetSummaryFromRow(
        const WorkSheetSummaryRow& row,
        CsvReader::FileContext& ctx);

    static std::optional<WorkSheetOrderInfo>
    buildWorkSheetOrderInfoFromRow(
        const WorkSheetOrderInfoRow& row,
        CsvReader::FileContext& ctx);
};