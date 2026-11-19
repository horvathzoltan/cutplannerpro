#pragma once

#include "rbcsv_schema_section.h"


struct RbCsvSchema
{
    RbCsvSchemaSection root;

    QHash<QString,RbCsvSchemaSection> sections;
};