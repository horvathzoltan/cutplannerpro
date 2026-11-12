#pragma once

#include <typeindex>


class ICollectionAccessor
{
public:

    virtual ~ICollectionAccessor() = default;

    virtual int Count(
        const void* collection) const = 0;

    virtual const void* ItemAt(
        const void* collection,
        int index) const = 0;

    virtual std::type_index ItemType() const = 0;
};