#pragma once

#include <QStack>

#include "meta/discoverypath.h"
#include "rbcsv/model/rbcsv_importnode.h"

struct ImportTreeContext
{
    QStack<RbCsvImportNode*> nodes;

    SchemaPath path;

    RbCsvImportNode* Current()
    {
        if(nodes.isEmpty())
        {
            return nullptr;
        }

        return nodes.top();
    }

    const RbCsvImportNode* Current() const
    {
        if(nodes.isEmpty())
        {
            return nullptr;
        }

        return nodes.top();
    }

    bool IsRoot() const
    {
        return nodes.size() <= 1;
    }

    int Depth() const
    {
        return path.Depth();
    }

    QString Path() const
    {
        return PathString(path);
    }

    void Push(
        const QString& sectionName,
        RbCsvImportNode* node)
    {
        path.Push(sectionName);
        nodes.push(node);
    }

    void Pop()
    {
        if(nodes.size() > 1)
        {
            nodes.pop();
            path.Pop();
        }
    }
};