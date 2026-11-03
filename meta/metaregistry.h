#pragma once

#include "imetadata.h"

#include <QMap>
#include <typeindex>


class MetaRegistry
{
private:
    inline static QMap<std::type_index, IMetaData*> _items;

public:

    static void Register(
        const std::type_index& type,
        IMetaData* meta)
    {
        _items.insert(type, meta);
    }

    static IMetaData* Find(
        const std::type_index& type)
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
};


inline bool HasMeta(const std::type_index& type)
{
    return MetaRegistry::Find(type) != nullptr;
}
