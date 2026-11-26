#pragma once

#include <QList>

#include "meta/metavalue.h"
#include "rbcsv/model/rbcsv_importnode.h"

QList<MetaValue>
NodeToMetaValues(
    const RbCsvImportNode& node);