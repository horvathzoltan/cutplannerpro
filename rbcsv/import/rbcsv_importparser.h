#pragma once

#include <QString>

#include "../model/rbcsv_document.h"

class RbCsvImportParser
{
public:

    static RbCsvDocument Parse(
        const QString& text);
};