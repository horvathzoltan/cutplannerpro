#pragma once

#include "common/logger.h"
#include "discoverypath.h"
#include "meta/imetadata.h"

class MetaDumpWorker
{
public:

    void EnterType(IMetaData* meta, const DiscoveryPath& path)
    {
        int depth = path.Depth();
        zInfo("enter: " + QString(depth * 2, ' ') + meta->baseTypeName());
    }

    void LeaveType(IMetaData* metaData,const DiscoveryPath& path)
    {
        int depth = path.Depth();
        zInfo("leave type: " +QString(depth * 2, ' ')+ metaData->baseTypeName());
    }

    // field:amit most dolgozunk fel
    // path.Current(): melyik composition alatt vagyunk
    void Field(IMetaData*,const MetaField& field, const DiscoveryPath& path)
    {
        int depth = path.Depth();
        zInfo("field: "+ QString((depth + 1) * 2, ' ') + field.name);
    }

    void EnterComposition(IMetaData* parent,const MetaFieldBase& field,IMetaData* child, const DiscoveryPath& path)
    {
        int depth = path.Depth();
        zInfo("composition: " + field.name + " path:" + path.Path());
    }

    void LeaveComposition(IMetaData* parent,const MetaFieldBase& field,IMetaData* child,const DiscoveryPath& path)
    {
        int depth = path.Depth();
        zInfo("leave composition: "+ field.name+ " path:"+ path.Path());
    }


};