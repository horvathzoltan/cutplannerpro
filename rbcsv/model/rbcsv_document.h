#pragma once

#include "rbcsv_importnode.h"

struct RbCsvDocument
{
    RbCsvImportNode schema;

    QList<RbCsvImportNode> data;

    QString ToString() const
    {
        QString out;

        out += "=== SCHEMA ===\n";
        out += schema.ToString();

        out += "\n\n=== DATA ===\n";

        int ix = 0;

        for(const auto& node : data)
        {
            out +=
                QString(
                    "\n--- ROOT[%1] ---\n")
                    .arg(ix++);

            out += node.ToString();
            out += "\n";
        }

        return out;
    }

    QString ToString_Summary() const
    {
        return
            QString(
                "schemaChildren=%1 dataRoots=%2")
                .arg(schema.children.size())
                .arg(data.size());
    }
};