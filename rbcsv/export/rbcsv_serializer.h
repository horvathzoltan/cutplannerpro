#pragma once

#include <QHash>
#include <QString>

#include <worksheetsummary/model/worksheetorderinfo.h>

#include "../model/rbcsv_schema.h"

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
        const RbCsvSchema& plan)
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
        const RbCsvSchema& plan,
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
        const RbCsvSchemaSection& section,
        const RbCsvSchema& plan,
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
                QString fn = field->toExportString();
                record.append(fn);
            }
            else
            {
                QVariant value;
                if(field->kind == MetaFieldKind::Composition)
                {
                    value = field->toExportString();
                }
                else if(field->kind == MetaFieldKind::Reference)
                {
                    value = field->toExportString();
                }
                else if(field->kind == MetaFieldKind::Value)
                {
                     value =currentObject ? field->GetValue((char*)currentObject):"NULL";

                    if(MetaLog::Serialization){
                        zInfo(
                            QString("SerializeSection: %1 -> %2 valid=%3")
                                .arg(field->name)
                                .arg(value.metaType().name())
                                .arg(value.isValid()));
                    }
                }

                record.append(RbCsvValueFormatter::ToString(field,value));

            }
        }

        // for(const auto& childKey : section.children)
        // {
        //     int pos =
        //         childKey.lastIndexOf('/');

        //     QString sectionName =
        //         pos < 0
        //             ? childKey
        //             : childKey.mid(pos + 1);

        //     record.append(
        //         "[" + sectionName + "]");
        // }

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
                            int count = a->collectionAccessor->Count(ptr);

                            if(count > 0)
                            {
                                out += it.value().sectionName;
                                out += "\n";
                            }

                            for(int i=0;i<count;i++)
                            {
                                const void* item =a->collectionAccessor->ItemAt(ptr,i);

                                SerializeSection(
                                    it.value(),
                                    plan,
                                    out,
                                    false,
                                    mode,
                                    item);
                            }

                            if(count > 0)
                            {
                                out += "END # ";
                                out += it.value().sectionName;
                                out += "\n";
                            }

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