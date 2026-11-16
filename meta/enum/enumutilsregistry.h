#pragma once

#include "ienumutils.h"

#include <QHash>
#include <typeindex>

class EnumUtilsRegistry
{
public:

    static void Register(
        IEnumUtils* utils);

    static IEnumUtils* Find(
        std::type_index type);

private:

    static QHash<
        std::type_index,
        IEnumUtils*>& Map();
};