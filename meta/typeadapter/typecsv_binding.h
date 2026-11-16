#pragma once

#include <QString>


template<typename T>
struct TypeCsvBinding
{
    static QString ToCsv(
        const void* ptr)
    {
        return
            T::toCsv(
                *reinterpret_cast<
                    const typename T::ValueType*>(
                    ptr));
    }

    static bool FromCsv(
        const QString& text,
        void* ptr)
    {
        *reinterpret_cast<
            typename T::ValueType*>(
            ptr)
            =
            T::fromCsv(text);

        return true;
    }
};