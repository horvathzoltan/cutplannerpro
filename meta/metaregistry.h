#pragma once

#include "imetadata.h"

#include <QMap>
#include <typeindex>


class MetaRegistry
{
private:
    inline static QMap<std::type_index, IMetaData*> _items;
    inline static bool _sealed = false;

public:

    static void Register( const std::type_index& type, IMetaData* meta)
    {
        if(_sealed)
        {
            zError("MetaRegistry is sealed");
            return;
        }

        _items.insert(type, meta);
    }

    static IMetaData* Find(const std::type_index& type)
    {
        auto it = _items.find(type);

        if(it == _items.end())
            return nullptr;

        return it.value();
    }

    static QList<IMetaData*> All()
    {
        return _items.values();
    }

    static void Finalize()
    {
        for(auto it = _items.begin(); it != _items.end(); ++it)
        {
            it.value()->Finalize();
        }
    }

    bool IsSealed() { return _sealed;}
    static void Seal(){_sealed = true;}

};


inline bool HasMeta(const std::type_index& type)
{
    return MetaRegistry::Find(type) != nullptr;
}


