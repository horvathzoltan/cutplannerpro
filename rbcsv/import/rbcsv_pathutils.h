#pragma once

#include <QString>


inline QString ParentPath(
    const QString& path)
{
    int pos =
        path.lastIndexOf('/');

    if(pos < 0)
    {
        return {};
    }

    return path.left(pos);
}

inline QString LeafName(
    const QString& path)
{
    int pos =
        path.lastIndexOf('/');

    if(pos < 0)
    {
        return path;
    }

    return path.mid(pos + 1);
}