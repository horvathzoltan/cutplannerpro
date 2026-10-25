#pragma once


namespace WorkSheetSummaryTableColumns
{

enum Column
{
    FileName = 0,
    Presence,
    RequestCount,
    Completed,
    Uncompleted,
    Undefined,
    Attention
};

}

namespace WorkSheetOrderColumns
{
enum Column
{
    ExternalReference = 0,
    OwnerName,
    DueDate,
    ProductType,
    RequiredColor,
    Progress
};
}