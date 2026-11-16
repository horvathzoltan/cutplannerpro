#include "typecsvadapter_registry.h"
#include "common/logger.h"

#include <QMap>

namespace
{

QMap<
    std::type_index,
    TypeCsvAdapter>&
Registry()
{
    static QMap<
        std::type_index,
        TypeCsvAdapter> map;

    return map;
}

}

void TypeCsvAdapterRegistry::Register(
    const std::type_index& type,
    const TypeCsvAdapter& adapter)
{
    zInfo(
        QString(
            "TypeCsvAdapter registered: %1")
            .arg(type.name()));


    Registry()[type] = adapter;
}

const TypeCsvAdapter*
TypeCsvAdapterRegistry::Find(
    const std::type_index& type)
{
    auto& map = Registry();

    auto it = map.find(type);

    if(it == map.end())
    {
        zInfo(
            QString(
                "TypeCsvAdapter NOT found: %1")
                .arg(type.name()));

        return nullptr;
    }

    zInfo(
        QString(
            "TypeCsvAdapter found: %1")
            .arg(type.name()));

    return &it.value();
}