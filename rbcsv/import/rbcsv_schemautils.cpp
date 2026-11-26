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

    const QString& target = parts[depth];

    auto it = node.children.find(target);

    if(it == node.children.end())
    {
        return nullptr;
    }

    const auto& children = it.value();

    if(children.isEmpty())
    {
        return nullptr;
    }

    return
        FindSchemaNodeRecursive(children.first(),parts,depth + 1);
}

} // end of namespace


const RbCsvImportNode*
FindSchemaNode(
    const RbCsvImportNode& root,
    const QString& path)
{
    if(path.isEmpty())
    {
        return &root;
    }

    QStringList parts = path.split('/', Qt::SkipEmptyParts);

    auto a =  FindSchemaNodeRecursive(root,parts,0);
    return a;
}