#include "enumutilsregistry.h"

QHash<
    std::type_index,
    IEnumUtils*>&
EnumUtilsRegistry::Map()
{
    static QHash<
        std::type_index,
        IEnumUtils*> map;

    return map;
}

void EnumUtilsRegistry::Register(
    IEnumUtils* utils)
{
    if(!utils)
        return;

    Map().insert(
        utils->GetEnumType(),
        utils);
}

IEnumUtils*
EnumUtilsRegistry::Find(
    std::type_index type)
{
    auto it =
        Map().find(type);

    if(it == Map().end())
        return nullptr;

    return it.value();
}