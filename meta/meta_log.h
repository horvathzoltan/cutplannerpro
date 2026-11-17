#pragma once

struct MetaLog
{
    // MetaData::AddField
    static inline bool BuildSchema = false;

    // MetaField::GetValue
    static inline bool GetValue = false;

    // CSV adapterek
    static inline bool Bindings = false;

    // Serializer
    static inline bool Serialization = false;

    // DiscoverTree
    static inline bool Discovery = false;
};