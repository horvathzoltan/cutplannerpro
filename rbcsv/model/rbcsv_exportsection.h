#pragma once

#include <QString>
#include <QStringList>
#include <meta/metafield.h>

struct RbCsvExportSection
{
    QString sectionName;

    QList<const MetaFieldBase*> fields;

    QList<QString> children;
};
