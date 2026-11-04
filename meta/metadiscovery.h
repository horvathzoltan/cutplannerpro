#pragma once

#include "imetadata.h"
#include "metaregistry.h"

template<typename Worker>
void DiscoverTree(
    IMetaData* meta,
    Worker& worker,
    int depth = 0)
{
    if(!meta)
        return;

    worker.EnterType( meta,depth);

    for(const auto& field : meta->fields())
    {
        IMetaData* child =MetaRegistry::Find(field.itemType);

        if(child)
        {
            worker.EnterComposition(meta,field,child,depth);
            DiscoverTree(child,worker,depth + 1);
        }
        else
        {
            worker.Field(meta,field,depth);
        }
    }

    worker.LeaveType(meta,depth);
}