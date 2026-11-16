#pragma once

#include <typeindex>

class IEnumUtils
{
public:
    virtual ~IEnumUtils() = default;

    virtual std::type_index
    GetEnumType() const = 0;

    virtual void
    RegisterConverters() = 0;
};