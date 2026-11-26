#pragma once

#include "rbcsv/model/rbcsv_importnode.h"

template<typename T>
bool Materialize(
    const RbCsvImportNode& node,
    T& object)
{
    auto& meta =
        T::Meta();

    for(const auto& field : meta.fieldsRef())
    {
        auto it =
            node.record.values.find(
                field.name);

        if(it ==
            node.record.values.end())
        {
            continue;
        }

        field.SetValue(
            reinterpret_cast<char*>(&object),
            QVariant(it.value()));
    }

    return true;
}

template<typename T>
T Materialize(
    const RbCsvImportNode& node)
{
    T result;

    Materialize(
        node,
        result);

    return result;
}