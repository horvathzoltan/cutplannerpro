#pragma once

#include <QString>
#include <typeindex>

struct TypeCsvAdapter
{
    std::type_index valueType =
        std::type_index(
            typeid(void));

    QString (*toCsv)(
        const void*);

    bool (*fromCsv)(
        const QString&,
        void*);
};