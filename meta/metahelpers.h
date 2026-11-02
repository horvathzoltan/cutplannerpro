#pragma once

#include "common/logger.h"
#include "meta/imetadata.h"
#include "metaregistry.h"
#include <QList>
#include <QMetaType>
#include <QString>
#include <QVariant>


struct FieldType{
    QMetaType declType;
    QMetaType keyType;
    QMetaType valueType;
};


class IdMegnev{
public:
    int id = -1;
    QString name;
    QString code;
};

struct IdMegnevIxs{
public:
    int id = -1;
    QList<int> megnev;
    int code = -1;
};

struct DataRowDefaultModel{
private:
    DataRowDefaultModel(){};
    QString _name;
    QList<IdMegnev> _values;
public:
    DataRowDefaultModel(const QString& n){ _name=n;};
    void SetName_(const QString& v){ _name = v;}
    void AddValue(const IdMegnev& v){ _values.append(v);}
    QString name(){return _name;}
    const QList<IdMegnev>& values(){return _values;}

    bool IsValid(){return !_name.isEmpty();}

    QString GetMegnev(int id)
    {
        for(auto&v:_values){
            if(v.id==id) return v.name;
        }
        return {};
    }

    // void SetName_(const QString& v, unsigned long l){
    //     Q_UNUSED(l);
    //     _name = v;
    // };


};

inline void DumpMetaTree(
    IMetaData* meta,
    const QString& indent = "")
{
    if(!meta)
        return;

    zInfo(
        indent
        + meta->baseTypeName());

    for(const auto& c :
         meta->compositions())
    {
        zInfo(
            indent
            + "  "
            + c.fieldName);

        IMetaData* child =
            MetaRegistry::Find(
                c.childType);

        DumpMetaTree(
            child,
            indent + "    ");
    }
}