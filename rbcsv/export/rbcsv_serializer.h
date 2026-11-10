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



                // for(const auto* a : section.fields)
                // {
                //     if(a->name == it.value().sectionName)
                //     {
                //         ptr = a->GetPtr((char*)currentObject);
                //         break;
                //     }
                // }

                for(const auto* a : section.fields){
                    if(a->name == childKey){
                        ptr = a->GetPtr((char*)currentObject);

                        if(childKey == "orders"){

                            // most itt vagy van a qt-ben erre megoldás, hogy egy kollekciónak tudjuk a pointerét, és tudjuk a típusát, akkor be tudjuk járni
                            // vagy nekem kell megoldani, hogy a kollekció típusát tudjuk, és tudjuk a pointerét, és akkor tudjuk bejárni egy helperrel, mondjuk

                            QList<WorkSheetOrderInfo>* ptr1 = reinterpret_cast<QList<WorkSheetOrderInfo>*>(ptr);

                            //QList<WorkSheetOrderInfo>* ptr3 = dynamic_cast<QList<a->type.registerType()>*>(ptr);

                            if(ptr1){
                                for(auto b: *ptr1){
                                    ptr = reinterpret_cast<char*>(&b);

                                    SerializeSection(
                                        it.value(),
                                        plan,
                                        out,
                                        true,
                                        mode,
                                        ptr);
                                }

                                out += "END # ";
                                out += it.value().sectionName;
                                out += "\n";

                                bejarva = true;
                                //WorkSheetOrderInfo* ptr2 = &(ptr1->first());
                                // if(ptr2){
                                //     ptr = reinterpret_cast<char*>(ptr2);
                                // }
                            }
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