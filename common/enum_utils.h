#pragma once

// 1) Enum érték generálása
#define ENUM_VALUE(name) name,

// 2) Array elem generálása
#define ENUM_ARRAY(name) EEE::name,

// 3) Elem-számláló
#define ENUM_COUNT(name) +1

// 4) A fő makró
#define zEnum(EnumName, ENUM_LIST) \
    enum class EnumName { \
        ENUM_LIST(ENUM_VALUE) \
};

// 5) Iterálható tömb generálása

#define CONCAT(a,b) a##b
#define UNIQUE_NAME(base) CONCAT(base, __LINE__)

#define ENUM_TOSTRING_CASE(name) case EEE::name: return #name;

#define zEnum_helpers(EnumName, ENUM_LIST) \
struct CONCAT(EnumName, Helpers) { \
        using EEE = EnumName; \
\
        static constexpr std::array<EnumName, (0 ENUM_LIST(ENUM_COUNT))> \
        values = { ENUM_LIST(ENUM_ARRAY) }; \
\
        static const char* toString(EnumName v) { \
            switch (v) { \
                ENUM_LIST(ENUM_TOSTRING_CASE) \
        } \
            return "<unknown>"; \
    } \
};