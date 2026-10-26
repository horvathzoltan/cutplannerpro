#pragma once


#include <cstddef>
enum class WorkSheetOrderColumn
{
    Ref = 0,
    Owner,
    Due,
    TypeSubtype,
    Color,
    Progress,

    Count
};

inline constexpr std::size_t ColumnCount = static_cast<std::size_t>(WorkSheetOrderColumn::Count);

inline constexpr std::size_t toColumnIndex( WorkSheetOrderColumn column)
{
    return static_cast<std::size_t>(column);
}

// jelezni, hogy valaki eltolta az oszloprendet:
static_assert( toColumnIndex( WorkSheetOrderColumn::Progress) == 5);

