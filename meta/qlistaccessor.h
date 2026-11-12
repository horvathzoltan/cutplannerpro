#pragma once

#include "icollectionaccessor.h"

#include <QList>



template<typename T>
class QListAccessor : public ICollectionAccessor
{
public:

    int Count(
        const void* collection) const override
    {
        const auto* list =
            static_cast<
                const QList<T>*>(
                collection);

        return list->size();
    }

    const void* ItemAt(
        const void* collection,
        int index) const override
    {
        const auto* list =
            static_cast<
                const QList<T>*>(
                collection);

        return &((*list)[index]);
    }

    std::type_index ItemType() const override
    {
        return typeid(T);
    }
};
