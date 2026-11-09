#pragma once

#include <QString>
#include "meta/imetadata.h"
#include "meta/metafield.h"
#include "meta/discoverypath.h"

class RbCsvTestValueProvider
{
public:

    QString Value(
        IMetaData*,
        const MetaFieldBase& field,
        const DiscoveryPath&)
    {
        return "<" + field.name + ">";
    }
};
