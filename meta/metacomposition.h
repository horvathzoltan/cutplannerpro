#pragma once

#include <QList>
#include <QString>
#include <typeindex>


struct MetaComposition
{
    int fieldIx = -1;

    QString fieldName;

    QString childTypeName;

    std::type_index childType =
        std::type_index(typeid(void));

};

template<typename T>
struct CollectionTraits
{
    static constexpr bool IsCollection = false;
};

// template<typename T>
// struct CollectionTraits<QVector<T>>
// {
//     static constexpr bool IsCollection = true;

//     using ItemType = T;
// };

template<typename T>
struct CollectionTraits<QList<T>>
{
    static constexpr bool IsCollection = true;

    using ItemType = T;
};