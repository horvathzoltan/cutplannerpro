#pragma once

#include <QString>

struct TypeCsvAdapter
{
    QString (*toCsv)(const void*);
    bool (*fromCsv)(
        const QString&,
        void*);
};
