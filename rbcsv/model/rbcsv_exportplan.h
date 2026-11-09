#pragma once

#include "rbcsv_exportsection.h"


struct RbCsvExportPlan
{
    RbCsvExportSection root;

    QHash<QString,RbCsvExportSection> sections;
};