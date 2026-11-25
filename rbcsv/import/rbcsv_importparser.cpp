#include "importtreecontext.h"
#include "rbcsv_importparser.h"
#include "common/logger.h"
#include "meta/discoverypath.h"
#include "databuildcontext.h"
#include "rbcsv_schemautils.h"

#include <QStringList>
#include <QStack>

#include "rbcsv/import/rbcsv_pathutils.h"
#include "datanodepath.h"

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

    const auto& schemaRecord =
        schemaNode.record;

    QStringList parts =
        line.split(
            ';',
            Qt::KeepEmptyParts);

    for(int i = 0;
         i < parts.size();
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

    ImportTreeContext ctx;

    ctx.nodes.push(&root);

    for(const QString& line : lines)
    {
        if(line.isEmpty())
        {
            continue;
        }

        if(line.startsWith("END"))
        {
            ctx.Pop();
            continue;
        }

        if(IsSectionBegin(line))
        {
            RbCsvImportNode node;

            node.sectionName = line;

            ctx.Current() ->children[line].append(node);

            RbCsvImportNode* child =&ctx.Current()->children[line].last();

            ctx.Push(line,child);

            continue;
        }

        ctx.Current()->record = BuildSchemaRecord(line);
    }

    return root;
}


static RbCsvImportNode&
ResolveNode(
    QList<RbCsvImportNode>& roots,
    const DataNodePath& path)
{
    Q_ASSERT(path.IsValid());
    Q_ASSERT(path.rootIndex >= 0);
    Q_ASSERT(path.rootIndex < roots.size());

    RbCsvImportNode* node = &roots[path.rootIndex];

    for(const auto& item : path.items)
    {
        auto childIt = node->children.find(item.collectionName);

        Q_ASSERT(childIt !=node->children.end());
        Q_ASSERT(item.index >= 0);
        Q_ASSERT(item.index <childIt->size());

        node = &(*childIt)[item.index];
    }

    return *node;
}

/*
schema által definiált szerkezetű
RbCsvImportNode fát épít.
*/
static QList<RbCsvImportNode>
ParseData(
    const QStringList& lines,
    const RbCsvImportNode& schemaRoot)
{
    QList<RbCsvImportNode> result;

    DataBuildContext ctx;

    SchemaPath path;

    for(const QString& line : lines)
    {
        if(line.isEmpty())
        {
            continue;
        }

        if(line.startsWith("END"))
        {
            QString leavingSection =
                PathString(path);

            path.Pop();

            zInfo(
                QString(
                    "LEAVE SECTION: %1")
                    .arg(
                        leavingSection));

            continue;
        }

        if(IsSectionBegin(line))
        {
            path.Push(line);

            const auto* schemaNode =
                FindSchemaNode(
                    schemaRoot,
                    PathString(path));

            zInfo(
                QString(
                    "ENTER SECTION: %1 -> %2")
                    .arg(
                        PathString(path))
                    .arg(
                        schemaNode
                            ? schemaNode->sectionName
                            : "NULL"));

            if(!schemaNode)
            {
                zWarning(
                    QString(
                        "Schema section not found: %1")
                        .arg(
                            PathString(path)));
            }

            continue;
        }

        const auto* schemaNode =
            FindSchemaNode(
                schemaRoot,
                PathString(path));

        if(!schemaNode)
        {
            zWarning(
                QString(
                    "No schema for path: %1")
                    .arg(
                        PathString(path)));

            continue;
        }

        RbCsvImportRecord record =
            BuildDataRecord(
                line,
                *schemaNode);

        if(record.values.isEmpty())
        {
            continue;
        }

        zInfo(
            QString("DATA RECORD path=%1 fields=%2").arg(PathString(path)).arg(record.values.size()));

        const QString currentPath =PathString(path);

        if(currentPath.isEmpty())
        {
            result.append(RbCsvImportNode{});
            RbCsvImportNode& root =result.last();
            root.record =record;
            ctx.lastRecordAtPath.clear();

            DataNodePath rootPath;
            rootPath.rootIndex =result.size() - 1;
            ctx.lastRecordAtPath[""] =rootPath;
            continue;
        }

        QString parentPath = ParentPath(currentPath);
        QString leaf =LeafName(currentPath);

        auto parentIt = ctx.lastRecordAtPath.find(parentPath);

        if(parentIt ==ctx.lastRecordAtPath.end())
        {
            zWarning(QString("Missing parent path: %1").arg(parentPath));

            continue;
        }

        RbCsvImportNode& parent =ResolveNode(result,parentIt.value());
        parent.children[leaf].append(RbCsvImportNode{});

        RbCsvImportNode& node =parent.children[leaf].last();
        node.sectionName =leaf;
        node.record =record;

        DataNodePath nodePath =parentIt.value();

        const int childIndex = static_cast<int>(parent.children[leaf].size() - 1);

        nodePath.items.append({leaf,childIndex});

        ctx.lastRecordAtPath[currentPath] = nodePath;
        zInfo(
            QString(
                "NODE CREATED path=%1 parent=%2")
                .arg(currentPath)
                .arg(parentPath));

    }

    return result;
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
    doc.data = ParseData( dataLines,doc.schema);
    zInfo("=== AFTER PARSE SCHEMA ===");

    const RbCsvImportNode* ordersSchema =
        FindSchemaNode(
            doc.schema,
            "orders");

    if(ordersSchema)
    {
        zInfo("=== ENTER BUILD DATA TEST ===");

        auto testRecord =
            BuildDataRecord(
                "2826;Generálkiv., Pécs;1;700",
                *ordersSchema);

        zInfo("=== BUILD DATA RETURNED ===");

        zInfo(
            QString(
                "BuildDataRecord fields=%1")
                .arg(
                    testRecord.values.size()));

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