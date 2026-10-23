#include "worksheetsummary_repository.h"

#include "common/filehelper.h"
#include "common/logger.h"

#include <QFile>
#include <QTextStream>
#include <QStringConverter>

#include <product/registry/product_subtype_registry.h>
#include <product/registry/product_type_registry.h>

#include <materials/registry/material_registry.h>

QList<QList<QVector<QString>>>
WorkSheetSummaryRepository::readMsffSections(
    const QString& filepath)
{
    QFile file(filepath);

    if (!file.open(
            QIODevice::ReadOnly |
            QIODevice::Text))
    {
        zWarning(
            "❌ Nem sikerült megnyitni az MSFF fájlt: "
            + filepath);

        return {};
    }

    QTextStream in(&file);

    in.setEncoding(
        QStringConverter::Utf8);

    auto sepResult =
        FileHelper::detectSeparatorMsff(
            &in);

    if (sepResult.hasError ||
        sepResult.separator.isNull())
    {
        zWarning(
            "❌ MSFF: szeparátor detektálás sikertelen.");

        return {};
    }

    file.seek(0);
    in.seek(0);

    auto allRows =
        FileHelper::parseCSV(
            &in,
            sepResult.separator,
            true);

    return FileHelper::splitSections(
        allRows,
        sepResult.headerLineCount);
}

std::optional<
    WorkSheetSummaryRepository::
    WorkSheetSummaryRow>
    WorkSheetSummaryRepository::
    convertRowToWorkSheetSummaryRow(
        const QVector<QString>& parts,
        CsvReader::FileContext& ctx)
{
    if (parts.size() != 4)
    {
        ctx.addError(
            ctx.currentLineNumber(),
            QString(
                "❌ Hibás worksheet parent sor. "
                "4 mező szükséges, kapott: %1")
                .arg(parts.size()));

        return std::nullopt;
    }

    WorkSheetSummaryRow row;

    row.filePath =
        parts[0].trimmed();

    row.fileName =
        parts[1].trimmed();

    row.fileHash =
        parts[2].trimmed();

    row.presence =
        parts[3].trimmed();

    return row;
}

std::optional<
    WorkSheetSummaryRepository::
    WorkSheetOrderInfoRow>
    WorkSheetSummaryRepository::
    convertRowToWorkSheetOrderInfoRow(
        const QVector<QString>& parts,
        CsvReader::FileContext& ctx)
{
    if (parts.size() != 12)
    {
        ctx.addError(
            ctx.currentLineNumber(),
            QString(
                "❌ Hibás worksheet order sor. "
                "10 mező szükséges, kapott: %1")
                .arg(parts.size()));

        return std::nullopt;
    }

    WorkSheetOrderInfoRow row;

    row.externalReference =
        parts[0].trimmed();

    row.ownerName =
        parts[1].trimmed();

    row.dueDate =
        parts[2].trimmed();

    row.productTypeCode =
        parts[3].trimmed();


    row.productSubtypeCode =
        parts[4].trimmed();

    row.requiredColorCode =
        parts[5].trimmed();

    row.requiredSurfaceCode =
        parts[6].trimmed();

    row.materialBarcode =
        parts[7].trimmed();

    row.requestCount =
        parts[8].trimmed().toInt();

    row.completedCount =
        parts[9].trimmed().toInt();

    row.uncompletedCount =
        parts[10].trimmed().toInt();

    row.undefinedCount =
        parts[11].trimmed().toInt();

    return row;
}

std::optional<WorkSheetSummary>
    WorkSheetSummaryRepository::
    buildWorkSheetSummaryFromRow(
        const WorkSheetSummaryRow& row,
        CsvReader::FileContext& ctx)
{
    Q_UNUSED(ctx)

    WorkSheetSummary summary;

    summary.filePath =
        row.filePath;

    summary.fileName =
        row.fileName;

    summary.fileHash =
        QByteArray::fromBase64(
            row.fileHash.toUtf8());

    summary.presence =
        WorkSheetPresenceUtils::fromCsv(
            row.presence);

    return summary;
}

std::optional<WorkSheetOrderInfo>
    WorkSheetSummaryRepository::
    buildWorkSheetOrderInfoFromRow(
        const WorkSheetOrderInfoRow& row,
        CsvReader::FileContext& ctx)
{
    WorkSheetOrderInfo info;

    info.externalReference =
        row.externalReference;

    info.ownerName =
        row.ownerName;

    info.dueDate =
        QDate::fromString(
            row.dueDate,
            "yyyy-MM-dd");

    if (!info.dueDate.isValid())
    {
        ctx.addError(
            ctx.currentLineNumber(),
            QString(
                "❌ Érvénytelen dueDate: %1")
                .arg(row.dueDate));

        return std::nullopt;
    }

    const auto* type =
        ProductTypeRegistry::instance()
            .findByCode(row.productTypeCode);

    const auto* subtype =
        ProductSubtypeRegistry::instance()
            .findByCode(row.productSubtypeCode);

    info.productTypeId =
        type ? type->id : QUuid();

    info.productSubtypeId =
        subtype ? subtype->id : QUuid();

    info.requiredColor =
        NamedColor(
            row.requiredColorCode);

    info.requiredSurface =
        SurfaceTypeUtils::fromCode(
            row.requiredSurfaceCode);

    if (info.productTypeId.isNull())
    {
        ctx.addError(
            ctx.currentLineNumber(),
            QString(
                "❌ Érvénytelen productTypeCode: %1")
                .arg(row.productTypeCode));

        return std::nullopt;
    }


    if (info.productSubtypeId.isNull())
    {
        ctx.addError(
            ctx.currentLineNumber(),
            QString(
                "❌ Érvénytelen productSubtypeCode: %1")
                .arg(row.productSubtypeCode));

        return std::nullopt;
    }

    const auto* mat =
        MaterialRegistry::instance()
            .findByBarcode(
                row.materialBarcode);

    info.materialId =
        mat
            ? mat->id
            : QUuid();

    info.requestCount =
        row.requestCount;

    info.completedCount =
        row.completedCount;

    info.uncompletedCount =
        row.uncompletedCount;

    info.undefinedCount =
        row.undefinedCount;

    return info;
}

bool WorkSheetSummaryRepository::parseMsffSections(
    const QList<QList<QVector<QString>>>& sections,
    WorkSheetSummaryRegistry& registry)
{
    for (const auto& sec : sections)
    {
        if (sec.isEmpty())
        {
            continue;
        }

        CsvReader::FileContext parentCtx(
            "worksheet_parent");

        auto parentRow =
            convertRowToWorkSheetSummaryRow(
                sec[0],
                parentCtx);

        if (!parentRow.has_value())
        {
            zWarning(
                "❌ Hibás worksheet parent sor.");

            return false;
        }

        auto parent =
            buildWorkSheetSummaryFromRow(
                parentRow.value(),
                parentCtx);

        if (!parent.has_value())
        {
            zWarning(
                "❌ Hibás worksheet objektum.");

            return false;
        }

        WorkSheetSummary summary =
            parent.value();

        for (int i = 1;
             i < sec.size();
             ++i)
        {
            CsvReader::FileContext childCtx(
                summary.fileName);

            auto orderRow =
                convertRowToWorkSheetOrderInfoRow(
                    sec[i],
                    childCtx);

            if (!orderRow.has_value())
            {
                zWarning(
                    "❌ Hibás worksheet order sor.");

                return false;
            }

            auto order =
                buildWorkSheetOrderInfoFromRow(
                    orderRow.value(),
                    childCtx);

            if (!order.has_value())
            {
                zWarning(
                    "❌ Hibás worksheet order objektum.");

                return false;
            }

            summary.orders.push_back(
                order.value());
        }

        registry.registerData(
            summary);
    }

    return true;
}

bool WorkSheetSummaryRepository::loadFromFile(
    WorkSheetSummaryRegistry& registry,
    const QString& filePath)
{
    auto sections =
        readMsffSections(
            filePath);

    if (sections.isEmpty())
    {
        return false;
    }

    registry.clear();

    return parseMsffSections(
        sections,
        registry);
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

    out.setEncoding(
        QStringConverter::Utf8);

    out
        << "filePath;"
           "fileName;"
           "fileHash;"
           "presence\n";

    out
        << "externalReference;"
           "ownerName;"
           "dueDate;"
           "productTypeCode;"
           "productSubtypeCode;"
           "requiredColorCode;"
           "requiredSurfaceCode;"
           "materialBarcode;"
           "requestCount;"
           "completedCount;"
           "uncompletedCount;"
           "undefinedCount\n\n";

    bool first = true;

    for (const auto& ws : registry.readAll())
    {
        if (!first)
        {
            out << "\n";
        }

        first = false;
        out
            << ws.filePath << ";"
            << ws.fileName << ";"
            << ws.fileHash.toBase64() << ";"
            << WorkSheetPresenceUtils::toCsv(
                   ws.presence)
            << "\n";

        for (const auto& order : ws.orders)
        {
            auto* type =
                ProductTypeRegistry::instance()
                    .findById(order.productTypeId);

            auto* subtype =
                ProductSubtypeRegistry::instance()
                    .findById(order.productSubtypeId);

            const MaterialMaster* mat =
                MaterialRegistry::instance()
                    .findById(
                        order.materialId);

            out
                << order.externalReference << ";"
                << order.ownerName << ";"
                << order.dueDate.toString(
                       "yyyy-MM-dd")
                << ";"
                << (type?type->code:"")
                << ";"
                << (subtype?subtype->code:"")
                << ";"
                << order.requiredColor.code()
                << ";"
                << SurfaceTypeUtils::toCode(
                       order.requiredSurface)
                << ";"
                << (mat?mat->barcode:"")
                << ";"
                << order.requestCount
                << ";"
                << order.completedCount
                << ";"
                << order.uncompletedCount
                << ";"
                << order.undefinedCount
                << "\n";
        }

        out << "\n";
    }

    return true;
}