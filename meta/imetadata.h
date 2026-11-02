#pragma once

#include "meta/metacomposition.h"
#include <QMap>
#include <QString>
#include <typeindex>

class IMetaData
{
public:
    virtual ~IMetaData() = default;

    virtual QString baseTypeName() const = 0;

    virtual const QList<MetaComposition>& compositions() const = 0;
};