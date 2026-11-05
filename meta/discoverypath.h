#pragma once

#include <QList>
#include <QString>

#include "metafield.h"

struct DiscoveryPath
{
    QList<MetaFieldBase> fields;

    const MetaFieldBase* Current() const
    {
        if(fields.isEmpty())
            return nullptr;

        return &fields.last();
    }

    const MetaFieldBase* Parent() const
    {
        if(fields.size() < 2)
            return nullptr;

        return &fields[fields.size() - 2];
    }

    QString Path() const
    {
        QStringList parts;

        for(auto field : fields)
        {

            parts.append(field.name);
        }

        return parts.join('/');
    }

    int Depth() const
    {
        return fields.size();
    }
};