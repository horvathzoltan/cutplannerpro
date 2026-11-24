#include "rbcsv_schemautils.h"

#include <QStringList>

namespace
{

const RbCsvImportNode*
FindSchemaNodeRecursive(
    const RbCsvImportNode& node,
    const QStringList& parts,
    int depth)
{
    if(depth >= parts.size())
    {
        return &node;
    }

    const QString& target =
        parts[depth];

    for(const auto& child : node.children)
    {
        if(child.sectionName == target)
        {
            return
                FindSchemaNodeRecursive(
                    child,
                    parts,
                    depth + 1);
        }
    }

    return nullptr;
}

}

const RbCsvImportNode*
FindSchemaNode(
    const RbCsvImportNode& root,
    const QString& path)
{
    if(path.isEmpty())
    {
        return &root;
    }

    QStringList parts =
        path.split(
            '/',
            Qt::SkipEmptyParts);

    return
        FindSchemaNodeRecursive(
            root,
            parts,
            0);
}