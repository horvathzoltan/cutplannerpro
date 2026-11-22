#include "rbcsv_importparser.h"
#include "common/logger.h"

#include <QStringList>
#include <QStack>

namespace
{

enum class ParseSection
{
    None,
    Schema,
    Data
};



static bool IsSectionBegin(
    const QString& line)
{
    return
        !line.isEmpty()
        && !line.contains(';')
        && !line.startsWith("END");
}

static RbCsvImportRecord BuildSchemaRecord(
    const QString& line)
{
    RbCsvImportRecord record;

    QStringList parts =
        line.split(
            ';',
            Qt::KeepEmptyParts);

    int fieldIx = 0;

    for(const QString& part : parts)
    {
        if(part.startsWith('['))
        {
            continue;
        }

        record.values.insert(
            QString::number(fieldIx++),
            part.trimmed());
    }

    return record;
}

static RbCsvImportRecord BuildDataRecord(
    const QString& line,
    const RbCsvImportNode& schemaNode)
{
    RbCsvImportRecord record;

    if(schemaNode.records.isEmpty())
    {
        return record;
    }

    const auto& schemaRecord =
        schemaNode.records.first();

    QStringList parts =
        line.split(
            ';',
            Qt::KeepEmptyParts);

    for(int i=0;
         i<parts.size();
         ++i)
    {
        QString schemaKey =
            QString::number(i);

        auto it =
            schemaRecord.values.find(
                schemaKey);

        if(it ==
            schemaRecord.values.end())
        {
            continue;
        }

        record.values.insert(
            it.value(),
            parts[i].trimmed());
    }

    return record;
}

/**/


static RbCsvImportNode ParseSchema(
    const QStringList& lines)
{
    RbCsvImportNode root;

    root.sectionName = "schema";

    QStack<RbCsvImportNode*> stack;

    stack.push(&root);

    for(const QString& line : lines)
    {
        if(line.isEmpty())
        {
            continue;
        }

        if(line.startsWith("END"))
        {
            if(stack.size() > 1)
            {
                stack.pop();
            }

            continue;
        }

        if(IsSectionBegin(line))
        {
            RbCsvImportNode node;

            node.sectionName = line;

            stack.top()->children.append(node);

            stack.push(
                &stack.top()->children.last());

            continue;
        }

        stack.top()->records.append(
            BuildSchemaRecord(line));
    }

    return root;
}

} //end of anonymous namespace

RbCsvDocument RbCsvImportParser::Parse(
    const QString& text)
{
    QStringList schemaLines;
    QStringList dataLines;

    ParseSection section =
        ParseSection::None;

    const QStringList lines =
        text.split(
            '\n',
            Qt::KeepEmptyParts);

    for(QString line : lines)
    {
        line =
            line.trimmed();

        if(line == "#SCHEMA")
        {
            section =
                ParseSection::Schema;
            continue;
        }

        if(line == "#DATA")
        {
            section =
                ParseSection::Data;
            continue;
        }

        switch(section)
        {
        case ParseSection::Schema:
            schemaLines.append(line);
            break;

        case ParseSection::Data:
            dataLines.append(line);
            break;

        default:
            break;
        }
    }


    RbCsvDocument doc;

    doc.schema = ParseSchema(schemaLines);

    zInfo("=== AFTER PARSE SCHEMA ===");

    if(!doc.schema.children.isEmpty())
    {
        zInfo("=== ENTER BUILD DATA TEST ===");

        const auto& ordersSchema =doc.schema.children.first();

        auto testRecord = BuildDataRecord("2826;Generálkiv., Pécs;1;700",ordersSchema);

        zInfo("=== BUILD DATA RETURNED ===");

        zInfo(QString("BuildDataRecord fields=%1").arg(testRecord.values.size()));

        for(auto it = testRecord.values.begin();
             it != testRecord.values.end();
             ++it)
        {
            zInfo(
                QString("%1=%2")
                    .arg(it.key())
                    .arg(it.value()));
        }


        QStringList values;

        for(auto it = testRecord.values.begin();
             it != testRecord.values.end();
             ++it)
        {
            values.append(
                it.key()
                + "="
                + it.value());
        }

        zInfo(
            "BuildDataRecord test:");

        zInfo(
            values.join(", "));
    }

    Q_UNUSED(dataLines)


    zInfo(
        QString(
            "RBCSV schema lines: %1")
            .arg(schemaLines.size()));

    zInfo(
        QString(
            "RBCSV data lines: %1")
            .arg(dataLines.size()));

    return doc;
}