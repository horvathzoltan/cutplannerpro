#pragma once

#include <QHash>
#include <QString>

#include <worksheetsummary/model/worksheetorderinfo.h>

#include "../model/rbcsv_exportplan.h"

#include "rbcsv_valueformatter.h"

class RbCsvSerializer
{
public:

    enum class SerializeMode
    {
        Schema,
        Data
    };

    static QString SerializeSchema(
        const RbCsvExportPlan& plan)
    {
        QString out;

        SerializeSection(
            plan.root,
            plan,
            out,
            false,
            SerializeMode::Schema,
            nullptr);

        return out;
    }


    static QString SerializeData(
        const RbCsvExportPlan& plan,
        const void* rootObject)
    {
        QString out;

        SerializeSection(
            plan.root,
            plan,
            out,
            false,
            SerializeMode::Data,
            rootObject);

        return out;
    }

private:

    static void SerializeSection(
        const RbCsvExportSection& section,
        const RbCsvExportPlan& plan,
        QString& out,
        bool writeHeader,
        SerializeMode mode,
        const void* currentObject)
    {
        if(writeHeader)
        {
            out += section.sectionName;
            out += "\n";
        }

        QStringList record;

        for(const auto* field : section.fields)
        {
            if(mode == SerializeMode::Schema)
            {
                record.append(field->name);
            }
            else
            {

                QVariant value =
                    currentObject ?
                    field->GetValue(
                        (char*)currentObject):
                                     "NULL";

                zInfo(
                    QString("SerializeSection: %1 -> %2 valid=%3")
                        .arg(field->name)
                        .arg(value.metaType().name())
                        .arg(value.isValid()));

                record.append(
                    RbCsvValueFormatter::ToString(
                        field,
                        value));
            }
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

            char* ptr = nullptr;

            bool bejarva = false;


            if(currentObject){

                int pos =
                    childKey.lastIndexOf('/');

                QString sectionName =
                    pos < 0
                        ? childKey
                        : childKey.mid(pos + 1);

                for(const auto* a : section.fields){
                    if(a->name == sectionName){
                        ptr = a->GetPtr((char*)currentObject);

                        if(a->collectionAccessor)
                        {
                            int count =
                                a->collectionAccessor
                                    ->Count(ptr);

                            for(int i=0;i<count;i++)
                            {
                                const void* item =
                                    a->collectionAccessor
                                        ->ItemAt(ptr,i);

                                SerializeSection(
                                    it.value(),
                                    plan,
                                    out,
                                    true,
                                    mode,
                                    item);
                            }

                            out += "END # ";
                            out += it.value().sectionName;
                            out += "\n";

                            bejarva = true;
                        }

                        break;
                    }
                }

                //if(!ptr) continue;
            }

            if(bejarva) continue;

            SerializeSection(
                it.value(),
                plan,
                out,
                true,
                mode,
                ptr);

            out += "END # ";
            out += it.value().sectionName;
            out += "\n";
        }
    }
};