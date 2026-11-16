#pragma once

#include "typecsv_binding.h"
#include "typecsvadapter.h"

#include <typeindex>


class TypeCsvAdapterRegistry
{
public:

    static void Register(
        const std::type_index& type,
        const TypeCsvAdapter& adapter);

    static const TypeCsvAdapter*
    Find(
        const std::type_index& type);


    template<typename Binding>
    static void RegisterBinding()
    {
        TypeCsvAdapter adapter;

        adapter.toCsv =
            &TypeCsvBinding<
                Binding>::ToCsv;

        adapter.fromCsv =
            &TypeCsvBinding<
                Binding>::FromCsv;

        TypeCsvAdapterRegistry::
            Register(
                std::type_index(
                    typeid(
                        typename Binding::ValueType)),
                adapter);
    }
};

