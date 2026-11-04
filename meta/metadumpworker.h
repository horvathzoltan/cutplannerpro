#pragma once

#include "common/logger.h"
#include "meta/imetadata.h"

class MetaDumpWorker
{
public:

    void EnterType(
        IMetaData* meta,
        int depth)
    {
        zInfo(
            QString(depth * 2, ' ')
            + meta->baseTypeName());
    }

    void Field(
        IMetaData*,
        const MetaFieldBase&,
        int)
    {
    }

    void EnterComposition(
        IMetaData*,
        const MetaFieldBase& field,
        IMetaData*,
        int depth)
    {
        zInfo(
            QString((depth + 1) * 2, ' ')
            + field.name);
    }

    void LeaveType(
        IMetaData*,
        int)
    {
    }
};