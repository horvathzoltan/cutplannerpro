#pragma once

#include <QHash>
#include <QList>
#include <QMap>
#include <QString>
#include <QStringList>


struct RbCsvImportRecord
{
    QHash<QString, QString> values;
};

struct RbCsvImportNode
{
    QString sectionName;

    RbCsvImportRecord record;

    QHash<QString,QList<RbCsvImportNode>> children;
};

