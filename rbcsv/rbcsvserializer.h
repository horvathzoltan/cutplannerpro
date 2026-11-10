#pragma once

#include <QHash>
#include <QString>

#include "model/rbcsv_exportplan.h"

class RbCsvSerializer
{
public:

    static QString SerializeSchema(
        const RbCsvExportPlan& plan)
    {
        QString out;

        auto fieldWriter =
            [](
                const MetaFieldBase* field)
        {
            return field
                       ? field->name
                       : QString{};
        };

        SerializeSection(
            plan.root,
            plan,
            out,
            false,
            fieldWriter);

        return out;
    }


    template<typename TFieldWriter>
    static QString SerializeWith(
        const RbCsvExportPlan& plan,
        TFieldWriter&& fieldWriter)
    {
        QString out;

        SerializeSection(
            plan.root,
            plan,
            out,
            false,
            std::forward<TFieldWriter>(
                fieldWriter));

        return out;
    }
private:

    template<typename TFieldWriter>
    static void SerializeSection(
        const RbCsvExportSection& section,
        const RbCsvExportPlan& plan,
        QString& out,
        bool writeHeader,
        TFieldWriter&& fieldWriter)
    {
        if(writeHeader)
        {
            out += section.sectionName;
            out += "\n";
        }

        QStringList record;

        for(const auto* field : section.fields)
        {
            record.append(
                fieldWriter(field));
        }

        for(const auto& childKey : section.children)
        {
            int pos =
                childKey.lastIndexOf('/');

            QString sectionName =
                pos < 0
                    ? childKey
                    : childKey.mid(pos + 1);

            record.append(
                "[" + sectionName + "]");
        }

        if(!record.isEmpty())
        {
            out += record.join(';');
            out += "\n";
        }

        for(const auto& childKey : section.children)
        {
            const auto it =
                plan.sections.find(childKey);

            if(it == plan.sections.end())
                continue;

            SerializeSection(
                it.value(),
                plan,
                out,
                true,
                fieldWriter);

            out += "END # ";
            out += it.value().sectionName;
            out += "\n";
        }
    }
};