#include "worksheetsummary_repository.h"

#include "common/logger.h"

#include <QFile>
#include <QTextStream>
#include <QStringConverter>

bool WorkSheetSummaryRepository::loadFromFile(
    WorkSheetSummaryRegistry& registry,
    const QString& filePath)
{
    if (filePath.isEmpty() ||
        !QFile::exists(filePath))
    {
        return false;
    }

    CsvReader::FileContext ctx(filePath);

    QVector<WorkSheetSummary> data =
        loadFromCsv_private(ctx);

    if (ctx.hasErrors())
    {
        zWarning(ctx.toString());
    }

    registry.clear();

    for (const auto& item : data)
    {
        registry.registerData(item);
    }

    return true;
}

QVector<WorkSheetSummary>
WorkSheetSummaryRepository::loadFromCsv_private(
    CsvReader::FileContext& ctx)
{
    switch (detectCsvVersion(ctx.filepath()))
    {
    case CSVVersion::V1:
        return CsvReader::readAndConvert<WorkSheetSummary>(
            ctx,
            convertRowToWorkSheetSummary,
            true);

    default:
        ctx.addError(
            0,
            "Ismeretlen WorkSheetSummary CSV formátum");

        return {};
    }
}

WorkSheetSummaryRepository::CSVVersion
WorkSheetSummaryRepository::detectCsvVersion(
    const QString& filePath)
{
    QFile file(filePath);

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        return CSVVersion::Unknown;
    }

    QTextStream in(&file);
    in.setEncoding(QStringConverter::Utf8);

    QString header =
        in.readLine().trimmed();

    QStringList cols =
        header.split(';', Qt::KeepEmptyParts);

    bool isV1 =
        cols.contains("filePath") &&
        cols.contains("fileName") &&
        cols.contains("fileHash") &&
        cols.contains("requestCount") &&
        cols.contains("completedCount") &&
        cols.contains("uncompletedCount") &&
        cols.contains("undefinedCount");

    return isV1
               ? CSVVersion::V1
               : CSVVersion::Unknown;
}

std::optional<WorkSheetSummaryRepository::WorkSheetSummaryRow>
WorkSheetSummaryRepository::convertRowToWorkSheetSummaryRow(
    const QVector<QString>& parts,
    CsvReader::FileContext& ctx)
{
    if (parts.size() < 7)
    {
        ctx.addError(
            ctx.currentLineNumber(),
            "Kevés adat");

        return std::nullopt;
    }

    WorkSheetSummaryRow row;

    row.filePath         = parts[0].trimmed();
    row.fileName         = parts[1].trimmed();

    row.fileHash         = parts[2].trimmed();

    row.requestCount     = parts[3].trimmed().toInt();

    row.completedCount   = parts[4].trimmed().toInt();

    row.uncompletedCount = parts[5].trimmed().toInt();

    row.undefinedCount   = parts[6].trimmed().toInt();

    return row;
}

std::optional<WorkSheetSummary>
WorkSheetSummaryRepository::convertRowToWorkSheetSummary(
    const QVector<QString>& parts,
    CsvReader::FileContext& ctx)
{
    auto rowOpt =
        convertRowToWorkSheetSummaryRow(
            parts,
            ctx);

    if (!rowOpt.has_value())
    {
        return std::nullopt;
    }

    const auto& row =
        rowOpt.value();

    WorkSheetSummary summary;

    summary.filePath =
        row.filePath;

    summary.fileName =
        row.fileName;

    summary.fileHash =
        QByteArray::fromBase64(
            row.fileHash.toUtf8());

    summary.requestCount =
        row.requestCount;

    summary.completedCount =
        row.completedCount;

    summary.uncompletedCount =
        row.uncompletedCount;

    summary.undefinedCount =
        row.undefinedCount;

    return summary;
}

bool WorkSheetSummaryRepository::saveToFile(
    const WorkSheetSummaryRegistry& registry,
    const QString& filePath)
{
    QFile file(filePath);

    if (!file.open(
            QIODevice::WriteOnly |
            QIODevice::Text))
    {
        return false;
    }

    QTextStream out(&file);

    out
        << "filePath;"
           "fileName;"
           "fileHash;"
           "requestCount;"
           "completedCount;"
           "uncompletedCount;"
           "undefinedCount\n";

    for (const auto& item : registry.readAll())
    {
        out
            << item.filePath << ";"
            << item.fileName << ";"
            << item.fileHash.toBase64() << ";"
            << item.requestCount << ";"
            << item.completedCount << ";"
            << item.uncompletedCount << ";"
            << item.undefinedCount
            << "\n";
    }

    file.close();

    return true;
}