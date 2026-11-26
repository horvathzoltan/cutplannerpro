#pragma once

#include <QHash>
#include <QString>

#include <rbcsv/import/datanodepath.h>

struct DataBuildContext
{
    QHash<QString,DataNodePath> lastRecordAtPath;
};