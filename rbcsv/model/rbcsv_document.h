#pragma once

#include "rbcsv_importnode.h"

struct RbCsvDocument
{
    RbCsvImportNode schema;

    QList<RbCsvImportNode> data;
};