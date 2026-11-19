#include "rbcsvimportdump.h"

namespace
{

void DumpNode(
    QString& out,
    const RbCsvImportNode& node,
    int depth)
{
    QString indent(
        depth * 2,
        ' ');

    out +=
        indent
        + "["
        + node.sectionName
        + "]"
        + QString(
              " records=%1 children=%2\n")
              .arg(node.records.size())
              .arg(node.children.size());

    int rowIndex = 0;

    for(const auto& record : node.records)
    {
        QStringList fields;

        for(auto it = record.values.begin();
             it != record.values.end();
             ++it)
        {
            fields.append(
                it.key()
                + "="
                + it.value());
        }

        out +=
            indent
            + QString("  row[%1] ")
                  .arg(rowIndex++)
            + fields.join(", ")
            + "\n";
    }

    for(const auto& child : node.children)
    {
        DumpNode(
            out,
            child,
            depth + 1);
    }
}

}

QString RbCsvImportDump::ToString(
    const RbCsvImportNode& node)
{
    QString out;

    DumpNode(
        out,
        node,
        0);

    return out;
}