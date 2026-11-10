#pragma once

#include <QList>

// Collection type introspection utilities.
// Used by MetaField to determine collection element type.
//
// Formerly part of MetaComposition prototype.
// Composition is now derived from field metadata and registry lookup.

 template<typename T>
 struct CollectionTraits
 {
     static constexpr bool IsCollection = false;

     using ItemType = void;
 };

template<typename T>
struct CollectionTraits<QList<T>>
{
    static constexpr bool IsCollection = true;

    using ItemType = T;
};