#pragma once

#include "meta/metafield.h"
#include <QMap>
#include <QString>


class IMetaData
{
public:
    virtual ~IMetaData() = default;

    virtual QString baseTypeName() const = 0;

    virtual QList<MetaFieldBase> fields() const = 0;
};