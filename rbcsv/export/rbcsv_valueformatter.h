#pragma once

#include <QString>
#include <QVariant>

class MetaField;

class RbCsvValueFormatter
{
public:

    static QString ToString(
        const MetaField* field,
        const QVariant& value);
};