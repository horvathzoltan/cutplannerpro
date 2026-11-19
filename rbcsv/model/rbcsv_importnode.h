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

    QList<RbCsvImportRecord> records;

    QList<RbCsvImportNode> children;
};

