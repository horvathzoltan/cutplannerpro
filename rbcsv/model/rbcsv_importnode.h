#pragma once

#include <QHash>
#include <QList>
#include <QMap>
#include <QString>
#include <QStringList>


struct RbCsvImportRecord
{
    QHash<QString, QString> values;

    QString ToString() const
    {
        QStringList fields;

        for(auto it = values.begin();
             it != values.end();
             ++it)
        {
            fields.append(
                it.key()
                + "="
                + it.value());
        }

        return fields.join(", ");
    }
};

struct RbCsvImportNode
{
    QString sectionName;

    RbCsvImportRecord record;

    QHash<QString,QList<RbCsvImportNode>> children;

    QString ToString_Summary() const
    {
        return
            QString(
                "[%1] fields=%2 childGroups=%3")
                .arg(sectionName)
                .arg(record.values.size())
                .arg(children.size());
    }

    QString ToString() const
    {
        QString out;

        out += "[" + sectionName + "]";

        QString rec = record.ToString();

        if(!rec.isEmpty())
        {
            out += "\n  " + rec;
        }

        for(auto it = children.begin();
             it != children.end();
             ++it)
        {
            out +=
                "\n  <"
                + it.key()
                + "> count="
                + QString::number(
                    it.value().size());

            for(const auto& child :
                 it.value())
            {
                QString childText =
                    child.ToString();

                QStringList lines =
                    childText.split('\n');

                for(const QString& line : lines)
                {
                    out +=
                        "\n    "
                        + line;
                }
            }
        }

        return out;
    }

};

