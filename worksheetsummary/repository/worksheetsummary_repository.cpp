#include "worksheetsummary_repository.h"

#include "common/filehelper.h"
#include "common/logger.h"
#include "rbcsv/import/rbcsv_materializer.h"
#include "rbcsv/import/rbcsv_metavalueadapter.h"
#include "rbcsv/model/rbcsv_document.h"

#include <QFile>
#include <QTextStream>
#include <QStringConverter>

#include <product/registry/product_subtype_registry.h>
#include <product/registry/product_type_registry.h>

#include <materials/registry/material_registry.h>

#include <meta/rbcsv_exportplanbuilder.h>

#include <rbcsv/export/rbcsv_serializer.h>

#include <rbcsv/import/rbcsv_importparser.h>

#include <rbcsv/model/rbcsvimportdump.h>


const QString WorkSheetSummaryRepository::MaterialSectionKey = "MAT:1";
const QString WorkSheetSummaryRepository::OrdersSectionKey = "ORDERS";


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

    // row.materialBarcode =
    //     parts[7].trimmed();

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

std::optional<WorkSheetOrderInfo> WorkSheetSummaryRepository::buildWorkSheetOrderInfoFromRow(
    const WorkSheetOrderInfoRow& row,CsvReader::FileContext& ctx)
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

    // const auto* mat =
    //     MaterialRegistry::instance()
    //         .findByBarcode(
    //             row.materialBarcode);

    // info.materialId =
    //     mat
    //         ? mat->id
    //         : QUuid();

    // RBCSV:
    // materials külön blokkból töltődik


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

    zInfo("\n***\n***\n***");
    zInfo("WorkSheetSummaryRepository::loadFromFile START");

    QFile f(filePath);

    if(f.open(
            QIODevice::ReadOnly |
            QIODevice::Text))
    {
        QTextStream in(&f);

        in.setEncoding(
            QStringConverter::Utf8);

        QString text = in.readAll();

        RbCsvDocument doc = RbCsvImportParser::Parse(text);

        zInfo("\n***\n***\n***");
        zInfo("RbCsvDocument dump:");

        zInfo(doc.ToString());




        if(!doc.data.isEmpty())
        {

            zInfo("WorkSheetSummaryRepository::loadFromFile Materialize");
            zInfo("\n***\n***\n***");

            const auto& root = doc.data.last();

            auto ordersIt =
                root.children.find("orders");

            if(ordersIt != root.children.end()
                && !ordersIt->isEmpty())
            {
                const auto& firstOrder =
                    ordersIt->first();

                auto materialsIt =
                    firstOrder.children.find(
                        "materials");

                if(materialsIt != firstOrder.children.end()
                    && !materialsIt->isEmpty())
                {
                    const auto& materialNode =
                        materialsIt->first();

                    zInfo("\n=== MATERIAL NODE TEST ===");

                    auto values =
                        NodeToMetaValues(
                            materialNode);

                    for(const auto& v : values)
                    {
                        zInfo(
                            QString("%1=%2 (%3)")
                                .arg(v.metaField_name)
                                .arg(v.value.toString())
                                .arg(v.value.metaType().name()));
                    }

                    WorkSheetMaterialInfo material;

                    bool ok = Materialize(materialNode,material);

                    zInfo(
                        QString(
                            "Materialize result=%1")
                            .arg(ok));

                    zInfo(
                        QString(
                            "Materialized length=%1")
                            .arg(
                                material.cutLengthMm));

                    const MaterialMaster* mat = MaterialRegistry::instance().findById(material.materialId);
                    auto msg = mat?mat->toReportLabel():"<unknown material>";


                    zInfo(
                        QString(
                            "Materialized materialId=%1 \n%2")
                            .arg(material.materialId.toString())
                            .arg(msg)
                        );



                }
            }
        }

        zInfo("WorkSheetSummaryRepository::loadFromFile END");
        zInfo("\n***\n***\n***");

    }



    return false;
}


bool WorkSheetSummaryRepository::saveToFile(
    const WorkSheetSummaryRegistry& registry,
    const QString& filePath)
{

    zInfo("WorkSheetSummaryRepository::saveToFile begin");

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

    RbCsvExportPlanBuilder builder;
    DiscoveryPath path;

    DiscoverTree(&WorkSheetSummary::Meta(),builder,path);

    auto schema_txt =RbCsvSerializer::SerializeSchema(builder.plan());
    out << "#SCHEMA\n\n";
    out << schema_txt;
    out << "\n#DATA\n\n";

    const auto& worksheets =
        registry.readAll();

    bool first = true;

    for(const auto& ws : worksheets)
    {
        if(!first)
        {
            out << "\n";
        }

        first = false;

        out << RbCsvSerializer::SerializeData(
            builder.plan(),
            &ws);
    }

    zInfo("WorkSheetSummaryRepository::saveToFile end");

    return true;
}



// load

