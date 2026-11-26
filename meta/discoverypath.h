#pragma once

#include "meta/metafield.h"
#include <QList>
#include <QString>
#include <QStringList>

template<typename T>
struct TreePath
{
    QList<T> items;

    const T* Current() const
    {
        if(items.isEmpty())
            return nullptr;

        return &items.last();
    }

    const T* Parent() const
    {
        if(items.size() < 2)
            return nullptr;

        return &items[items.size() - 2];
    }

    int Depth() const
    {
        return items.size();
    }

    bool IsEmpty() const
    {
        return items.isEmpty();
    }

    void Push(const T& item)
    {
        items.append(item);
    }

    void Pop()
    {
        if(!items.isEmpty())
        {
            items.removeLast();
        }
    }

    template<typename NameGetter>
    QString Path(NameGetter getName) const
    {
        QStringList parts;

        for(const auto& item : items)
        {
            parts.append(
                getName(item));
        }

        return parts.join('/');
    }
};

using DiscoveryPath = TreePath<MetaFieldBase>;
using SchemaPath = TreePath<QString>;


inline QString PathString(
    const DiscoveryPath& path)
{
    return path.Path(
        [](const MetaFieldBase& field)
        {
            return field.name;
        });
}

inline QString PathString(
    const SchemaPath& path)
{
    return path.Path(
        [](const QString& s)
        {
            return s;
        });
}