#pragma once

#include "common/logger.h"
#include "icollectionaccessor.h"
#include "meta/typeadapter/typecsv_adapter.h"
#include "meta_log.h"
#include "metavalue.h"
#include <QMetaType>
#include <typeindex>

enum class MetaFieldKind
{
    Value,
    Composition,
    Reference
};

struct MetaFieldAdapters
{
    const TypeCsvAdapter* csvBinding = nullptr;
};

struct MetaFieldBase{
public:
    QString name;
    QMetaType type;

    std::type_index declaredType =
        std::type_index(typeid(void));

    std::type_index itemType =
        std::type_index(typeid(void));

    MetaFieldAdapters adapters;

    RefType refType=RefType::None;

    QString ToString(){return name+' '+type.name();}

    const ICollectionAccessor* collectionAccessor = nullptr;

    MetaFieldKind kind = MetaFieldKind::Value;

    QString toExportString() const{
        if(kind == MetaFieldKind::Composition) return "[" + name + "]";
        return name;
    }
};

// ez megy ki az UI felé
struct MetaField : public MetaFieldBase{
public:
    QString wcode;
    int _offset;
    char* GetPtr(char* s) const { return s+ _offset; }

    // ez megy ki az UI felé
    MetaValue GetMetaValue(char* s) const
    {
        //  001c3 a megyére [5] invalid qvariant row widget keletkezik

        // if(name == "countyId")
        // {
        //       zInfo("countyId");
        //  }
        MetaValue mv(name, wcode, type, refType);
        //  001c2 a megyére az optional int mivel nincs értéke, invalid qvariant lesz az eredmény, és a row widget is invalid lesz
        //és elvész a típus - a metavalueba kell egy metatype mező
        mv.value = GetValue(s);
        return mv;
    }

    QVariant GetValue(char* s) const
    {
        char* ptr = GetPtr(s);

        if(MetaLog::GetValue)
        {
            zInfo(
                QString("GetValue type=%1 id=%2")
                    .arg(type.name())
                    .arg(type.id()));
        }
        if(adapters.csvBinding)
        {
            QString text =
                adapters.csvBinding->toCsv(ptr);

            if(MetaLog::Bindings){
            zInfo(
                QString(
                    "field adapter %1 -> %2")
                    .arg(wcode)
                    .arg(text));
            }
            return text;
        }


        if(type.id()==QMetaType::QVariant){
            //QMetaType qvm = type;
            //QVariant v(qvm, ptr);
            qWarning("Do not use QVariant in model!");
            // QVariant* p = (QVariant*)s;

            // QVariant v(*p);
            // return v;
            return {};
        }
        if(type.id()<QMetaType::User){
            QVariant v(type, ptr);

            if(MetaLog::GetValue)
            {
                zInfo(
                    QString("builtin valid=%1 type=%2")
                        .arg(v.isValid())
                        .arg(v.metaType().name()));
            }
            return v;
        }


        return {};

    }


    bool SetValue(
        char* object,
        const QVariant& value) const
    {
        char* ptr =
            GetPtr(object);

        if(adapters.csvBinding)
        {
            QString text =
                value.toString();

            return
                adapters.csvBinding
                    ->fromCsv(
                        text,
                        ptr);
        }

        if(type.id() == QMetaType::QVariant)
        {
            zError(
                QString(
                    "SetValue QVariant field not supported: %1")
                    .arg(name));

            return false;
        }

        const QMetaType sourceType =
            value.metaType();

        if(!QMetaType::canConvert(
                sourceType,
                type))
        {
            zWarning(
                QString(
                    "No converter from %1 to %2 field=%3")
                    .arg(sourceType.name())
                    .arg(type.name())
                    .arg(name));

            return false;
        }

        bool ok =
            QMetaType::convert(
                sourceType,
                value.constData(),
                type,
                ptr);

        if(!ok)
        {
            zWarning(
                QString(
                    "Convert failed from %1 to %2 field=%3")
                    .arg(sourceType.name())
                    .arg(type.name())
                    .arg(name));
        }

        return ok;
    }

};
