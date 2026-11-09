#pragma once

#include "imetadata.h"
#include "meta/discoverypath.h"
#include "metaregistry.h"

template<typename Worker>
void DiscoverTree(
    IMetaData* meta,
    Worker& worker,
    DiscoveryPath& path)
{
    if(!meta)
        return;

    worker.EnterType(meta, path);

    for(const auto& field : meta->fieldsRef())
    {
        IMetaData* child =
            MetaRegistry::Find(field.itemType);

        // EnterComposition: path points to the parent section.
        // LeaveComposition: path still points to the child section.

        if(child)
        {
            worker.EnterComposition(
                meta,
                field,
                child,
                path);

            path.fields.append(field);

            DiscoverTree(
                child,
                worker,
                path);

            worker.LeaveComposition(
                meta,
                field,
                child,
                path);

            path.fields.removeLast();
        }
        else
        {
            worker.Field(
                meta,
                field,
                path);
        }
    }

    worker.LeaveType(
        meta,
        path);
}