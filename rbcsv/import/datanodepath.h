#pragma once

#include <QList>
#include <QString>

struct DataNodePathItem
{
    QString collectionName;
    int index = -1;
};

struct DataNodePath
{
    int rootIndex = -1;

    QList<DataNodePathItem> items;

    bool IsValid() const
    {
        return rootIndex >= 0;
    }
};