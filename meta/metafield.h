#pragma once

#include "common/logger.h"
#include "metavalue.h"
#include <QMetaType>
#include <typeindex>


struct MetaFieldBase{
public:
    QString name;
    QMetaType type;

    std::type_index declaredType =
        std::type_index(typeid(void));

    std::type_index itemType =
        std::type_index(typeid(void));

    RefType refType=RefType::None;

    QString ToString(){return name+' '+type.name();}

    bool IsCollection() const
    {
        return itemType != std::type_index(typeid(void));
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
        if(type.id()==QMetaType::QVariant){
            //QMetaType qvm = type;
            //QVariant v(qvm, ptr);
            qWarning("Do not use QVariant in model!");
            // QVariant* p = (QVariant*)s;

            // QVariant v(*p);
            // return v;
            return {};
        } else if(type.id()<QMetaType::User){
            QVariant v(type, ptr);
            return v;
        } else {
            QMetaType qvm = QMetaType(QMetaType::QVariant);
            bool cc = QMetaType::canConvert(type, qvm);
            QVariant v;
            if(cc){
                bool ok = QMetaType::convert(type, ptr, qvm, &v);
                if(!ok){
                    zInfo("cc err");
                }
            }
            return v;
        }
    }

};
