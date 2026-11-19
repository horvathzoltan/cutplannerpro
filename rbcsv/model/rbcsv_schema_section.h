#pragma once

#include <QString>
#include <QStringList>
#include <meta/metafield.h>

struct RbCsvSchemaSection
{
    QString sectionName;

    QList<const MetaField*> fields;

    QList<QString> children;
};
