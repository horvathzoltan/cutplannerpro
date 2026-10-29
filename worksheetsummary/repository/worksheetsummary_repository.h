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

        int quantity = 0;

        int fullWidthMm = 0;
        int fullHeightMm = 0;

        QString dueDate;

        QString productTypeCode;
        QString productSubtypeCode;

        QString calcModeCode;

        QString requiredColorCode;
        QString requiredSurfaceCode;

        int requestCount = 0;
        int completedCount = 0;
        int uncompletedCount = 0;
        int undefinedCount = 0;
    };

    struct WorkSheetMaterialInfoRow
    {
        QString materialBarcode;
        int cutLengthMm = 0;
    };

    static const QString MaterialSectionKey;
    static const QString OrdersSectionKey;

    enum class NodeKind{ Row, Section};

    struct ParserState
    {
        NodeKind kind;
        int fieldIndex = -1;
    };

    struct FieldDescriptor
    {
        int index;
        QString name;
    };

    static void writeWorksheetRow(QTextStream& out,const WorkSheetSummary& ws);
    static void writeOrderRow(QTextStream& out, const WorkSheetOrderInfo& order);
    static void writeMaterialSection(QTextStream& out, const QVector<WorkSheetMaterialInfo>& materials);

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

private:

    static std::optional<WorkSheetMaterialInfoRow>
    convertRowToWorkSheetMaterialInfoRow(
        const QVector<QString>& parts,
        CsvReader::FileContext& ctx);

    static std::optional<WorkSheetMaterialInfo>
    buildWorkSheetMaterialInfoFromRow(
        const WorkSheetMaterialInfoRow& row,
        CsvReader::FileContext& ctx);




};