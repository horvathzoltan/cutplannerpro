#pragma once

#include <QList>
#include <QMetaType>
#include <QString>
#include <QVariant>
#include "common/logger.h"
#include "collectiontraits.h"
#include "metafield.h"
#include "metavalue.h"
#include "metahelpers.h"
#include "metareference.h"
#include "imetadata.h"
#include "metaregistry.h"
#include "qlistaccessor.h"

//#define AddMetaField(b) _meta.AddField(#b, QMetaType::fromType<decltype(_meta._instance.b)>(), (char*)(&_meta._instance.b));
#define AddMetaField(b) _meta.AddField< decltype(_meta._instance.b) >( #b, QMetaType::fromType<decltype(_meta._instance.b)>(), (char*)(&_meta._instance.b))
#define AddMetaBase(b) _meta.AddBaseName(#b, sizeof(b));

//#define AddRowToField(b,c) Add_RowToField(#b, c); {(void)_meta._instance.b;}
#define FieldName(t, b) TypeHelper::GetTypeName_(QStringLiteral(#b), sizeof(t::Meta()._instance.b))
#define AddRowToField(a,b,c) if(_meta.Contains(#b)){a.Add_RowToField(#b, c); {(void)_meta._instance.b;}} else {zWarning(QStringLiteral("MetaField is not exists: ")+#b);}

//#define SetName(t, b) SetName_(QStringLiteral(#b), sizeof(t::Meta()._instance.b))
#define SetName(t, b) SetName_(FieldName(t,b))
#define AddMetaReference_1N(t, b, r, c) AddMetaRef_<r>(FieldName(t,b), #r, FieldName(r,c), RefType::R_1N)
#define AddMetaReference_NM(t, b, r, c) AddMetaRef_<r>(FieldName(t,b), #r, FieldName(r,c), RefType::R_NM)
#define DeleteMetaReference(t, b, r) DeleteMetaRef_<r>(FieldName(t,b))

#define Copy_DataRowDefaultModel(v, t, b) Copy_DataRowDefaultModel_(v, FieldName(t,b))

#define SetMetaIdMegnev(t, a, b) setMetaIdMegnev(FieldName(t,a), FieldName(t,b))
#define AddMegnev(t, a) addMetaIdMegnev(FieldName(t,a))
#define AddCode(t, a) addMetaIdCode(FieldName(t,a))

template<typename T>
class MetaData : public IMetaData{
    QList<MetaField> _fields;
    IdMegnevIxs _idMegnevIxs;
    bool _verbose = true;
    bool _initialized = false;

public:
    T _instance;

    bool isInitialized() const
    {
        return _initialized;
    }

    void setInitialized()
    {
        _initialized = true;
    }

    QString baseTypeName() const override
    {
        return _baseName;
    }

    void RegisterSelf()
    {
        MetaRegistry::Register(
            std::type_index(typeid(T)),
            this);
    }

    //QVariantList _references;
    //QList<RefBase> _references;

    //TODO ezt később ki kell szervezni, a meta mint típusleíró önmagában megáll, nincs szüksége referenciálra, az csak egy ráépülő szolgáltatás kell legyen majd

    // TODO:
    // MetaData legyen teljesen független a referencia-rendszertől.
    // RefContainer és AddMetaRef_* kerüljön külön MetaReference komponensbe.
    // A composition nem reference, hanem a típusrendszerből felismerhető kapcsolat.
    /*
template<typename T>
class MetaModel
{
public:
    MetaData<T> data;
    MetaReferences references;
};
*/
    RefContainer _refcontainer;

    // QList<MetaFieldBase> fields() const override
    // {
    //     QList<MetaFieldBase> m;

    //     for(const auto& f : _fields)
    //         m.append(f);

    //     return m;
    // }

    const QList<MetaField>& fieldsRef() const override
    {
        return _fields;
    }

    template<typename R>
    Ref<R>* GetRef2(const QString& f)
    {
        int fIx = GetMetaFieldIx(f);
        return _refcontainer.Get<R>(fIx);
    }

    template<typename R>
    QString GetRef_FieldName(const QString& f)
    {
        int fIx = GetMetaFieldIx(f);
        Ref<R>* r1 = T::Meta().template GetRef2<R>(fIx);
        if(!r1) return {};
        int rIx = r1->fieldIx();
        if(rIx<0) return{};
        MetaField a = _fields[rIx];
        return a.name;
    }

    template<typename R>
    void AddMetaRef_(const QString& f, const QString& rt, const QString& rf, RefType refType){
        int fIx = GetMetaFieldIx(f);
        _fields[fIx].refType = refType;
        return _refcontainer.Add<R>(fIx, rt, rf, refType);
    };

    template<typename R>
    void DeleteMetaRef_(const QString& f){
        int fIx = GetMetaFieldIx(f);

        _refcontainer.Delete<R>(fIx);
    }

    // void DeleteMetaRef_All(){
    //     _refcontainer.DeleteMetaRef_All();
    // }

    QString _baseName;
    QString _baseWcode;

    void SetVerbose(bool v){_verbose = v;};

    ~MetaData(){
        //T::DeleteRefs();
    }
    // QString GetFieldName(const QString& name, char* field_ptr){
    //      return name;
    //  }

    template<typename FieldType>
    void AddField(const QString& name, const QMetaType& t, char* field_ptr)
    {
        // if(name == "alimedCode"){
        //       zInfo("alimedCode megvan");
        // }
        if(t.id() == QMetaType::QVariant){
            zWarning("AddField: Do not use QVariant "+name+" in model!");
        }
        MetaField f;
        f.name = name;
        f.wcode = _baseWcode+"::"+f.name;
        f.type = t;

        f.declaredType = std::type_index(typeid(FieldType));

        zInfo(QString("declared=") + QString(typeid(FieldType).name()));

        if constexpr(CollectionTraits<FieldType>::IsCollection)
        {
            using ItemType = typename CollectionTraits<FieldType>::ItemType;
            f.itemType = std::type_index(typeid(ItemType));
            zInfo(QString("item=")+ QString(typeid(ItemType).name()));

            static QListAccessor<ItemType> accessor;
            f.collectionAccessor = &accessor;
        }

        if constexpr(std::is_enum_v<FieldType>)
        {
            //RegisterEnumConverters<FieldType>();

            zInfo(
                QString("ENUM FIELD %1 type=%2")
                    .arg(name)
                    .arg(typeid(FieldType).name()));
        }

        int offset = field_ptr - (char*)(&_instance);
        f._offset = offset;

        if(_verbose){
            QString msg = QStringLiteral("AddField: ")
            +QString::number(offset)
                +" "+t.name() + '('+QString::number(t.sizeOf())+")"
                +" "+name
                +" to "+ _baseName;
            zInfo(msg);
        }
        _fields.append(f);
    }

    bool Contains(const QString& name)
    {
        for(auto&a:_fields) if(a.name == name) return true;
        return false;
    }

    // Gets field names separated with comma
    // SqlRepo: Get, GetAll
    QString GetFieldNames(){
        if(_fields.isEmpty()) return {};
        QString e;
        for(auto&a:_fields){
            //if(a.name.toLower()=="id") continue;
            if(!e.isEmpty()) e+=",";
            e+=a.name;
        }
        return e;
    }

    void AddBaseName(char const *y, unsigned long){
        _baseName = QString(y);
        _baseWcode = "WCodes::"+_baseName;
    }

    // QString GetBaseTypeName() {
    //     return _baseName;
    // }

    // QString GetFieldList_UPDATE(){
    //     if(_fields.isEmpty()) return {};
    //     QString e;
    //     int i = 0;
    //     for(auto&a:_fields){
    //         if(a.name.toLower()=="id") continue;
    //         if(!e.isEmpty()) e+=",";
    //         e+=a.name+"=: "+a.name;
    //     }
    //     return e;
    // }

private:
    MetaField* GetMetaField(const QString& name){
        for(auto&a:_fields){
            if(a.name==name) return &a;
        }
        return nullptr;
    }

public:
    // AddMetaRef-ben használjuk az indexét
    int GetMetaFieldIx(const QString& name){
        int L= _fields.count();
        for(int i=0;i<L;i++){
            if(_fields[i].name==name) return i;
        }
        return -1;
    }

    // Parses T from a list of MetaValue
    // SqlRepo: Get, GetAll
    T FromMetaValues(const QList<MetaValue>& metaValues){
        T s;
        for(auto&m:metaValues){
            // if(m.metaField_name == "postalCode"){
            //       zInfo("postalCode megvan");
            // }
            MetaField* f = GetMetaField(m.metaField_name);
            if(f){
                char* ptr = f->GetPtr((char*)&s);
                auto typeId = f->type.id();

                if(typeId==QMetaType::QVariant){
                    qWarning("Do not use QVariant in model!");
                }
                else if(typeId < QMetaType::User)
                {
                    // mivel a konverter QMetaTypeok közt működik
                    // ha a qvariant egyszerű típusú értéket tárol
                    // QVariant értékét kell átadni a konverternek
                    // kell konvertálni
                    QMetaType qvm = m.value.metaType();

                    QString msg2 = QString(qvm.name())+" to "+ f->type.name();

                    bool cc = QMetaType::canConvert(qvm, f->type);
                    if(cc)
                    {
                        QMetaType::convert(qvm, m.value.constData(), f->type, ptr);
                    } else{
                        QString msg = QStringLiteral("No converter registered from ") +msg2;
                        zWarning(msg);
                    }
                }
                else
                {
                    // mivel a konverter QMetaTypeok közt működik
                    // és mivel az optionalból qvariantot csinálunk
                    // ezért a qvariantot MAGÁT kell átadni a konverternek
                    // QVariant értékű QVariantoit user típusba
                    QMetaType qvm = QMetaType(QMetaType::QVariant);

                    QMetaType qvm2 = m.value.metaType();

                    QString msg2 = QString(qvm.name())+"("+qvm2.name()+") to "+ f->type.name();

                    bool cc = QMetaType::canConvert(qvm, f->type);
                    if(cc)
                    {
                        QMetaType::convert(qvm, &m.value, f->type, ptr);
                    } else{
                        QString msg = QStringLiteral("No converter registered from ") +msg2;
                        zWarning(msg);
                    }
                }
            }
        }
        return s;
    }

    QList<MetaValue> ToMetaValues(const T& s){
        QList<MetaValue> m;

        for (MetaField &f : _fields) {
            MetaValue mv =  f.GetMetaValue((char*)(&s));
            m.append(mv);
        }

        return m;
    }

    QList<QList<MetaValue>> ToMetaValueList(const QList<T>& values){
        QList<QList<MetaValue>> e;

        for(auto&a:values)
        {
            QList<MetaValue> m = ToMetaValues(a);
            e.append(m);
        }

        return e;
    }

    // QList<SQLHelper::SQLParam> ToSQLParams(const T* s){
    //     QList<SQLHelper::SQLParam> m;
    //     if(s){
    //         int i=1;
    //         for (MetaField &f : _fields) {
    //             bool isId = f.name.toLower()=="id";
    //             MetaValue mv =  f.GetMetaValue((char*)s);

    //             SQLHelper::SQLParam p;
    //             p.paramName = isId?"id":"p"+QString::number(i++);
    //             p.fieldName = mv.metaField_name;
    //             p.fieldValue = mv.value;
    //             m.append(p);
    //         }
    //     }
    //     return m;
    // }

    // QList<MetaFieldBase> ToMetaFieldBases()
    // {
    //     QList<MetaFieldBase> m;
    //     for (auto&f : _fields) {
    //         m.append(f);
    //     }
    //     return m;
    // }

    // struct MetaIdMegnevModel{
    // private:
    //     int _ixId;
    //     QList<int> _ixMegnevs;
    //     int _ixCode;
    //     MetaData* _m;
    // public:
    //     MetaIdMegnevModel(MetaData* p, int ix_id, int ix_m){
    //         _ixId = ix_id;
    //         _ixMegnevs.append(ix_m);
    //         _ixCode = -1;
    //         _m = p;
    //     }
    //     void addMegnev(const QString& megnev){
    //         int ix_m = _m->GetMetaFieldIx(megnev);
    //         _ixMegnevs.append(ix_m);
    //     }
    //     void addCode(const QString& code){
    //         int ix_m = _m->GetMetaFieldIx(code);
    //         _ixCode=ix_m;
    //     }

    //     int ixId(){return _ixId;}
    //     QList<int> ixMegnevs(){return _ixMegnevs;}
    //     int ixCode(){return _ixCode;}
    // };

    // MetaIdMegnevModel GetMetaIdMegnevModel(const QString& id, const QString& m){
    //     int ix_id = GetMetaFieldIx(id);
    //     int ix_m = GetMetaFieldIx(m);

    //     return MetaIdMegnevModel(this, ix_id, ix_m);
    // }

    // void MetaIdMegnevIndex(MetaIdMegnevModel a){
    //     _idMegnevIxs.id = a.ixId();
    //     _idMegnevIxs.megnev = a.ixMegnevs();
    //     _idMegnevIxs.code = a.ixCode();
    // }

    void setMetaIdMegnev(const QString& id, const QString& m){
        _idMegnevIxs.id = GetMetaFieldIx(id);
        _idMegnevIxs.megnev = {GetMetaFieldIx(m)};
        _idMegnevIxs.code =-1;
    }

    void addMetaIdMegnev(const QString& m){
        int ix = GetMetaFieldIx(m);
        _idMegnevIxs.megnev.append(ix);
    }

    void addMetaIdCode(const QString& m){
        int ix = GetMetaFieldIx(m);
        _idMegnevIxs.code = ix;
    }


    // void MetaIdMegnevIndex(int indexId, const QList<int> m, int c){
    //     _idMegnevIxs.id = indexId;
    //     _idMegnevIxs.megnev = m;
    //     _idMegnevIxs.code = c;
    // }

    IdMegnev ToIdMegnev(const T* s){
        IdMegnev e;
        if(_idMegnevIxs.id>-1){
            MetaField idf = _fields[_idMegnevIxs.id];
            MetaValue mv =  idf.GetMetaValue((char*)s);
            e.id = mv.value.toInt();
        }

        e.name.clear();
        for(auto &megnev:_idMegnevIxs.megnev){
            if(megnev>-1){
                MetaField idn = _fields[megnev];
                MetaValue mv2 =  idn.GetMetaValue((char*)s);
                if(!e.name.isEmpty()) e.name+=",";
                e.name += mv2.value.toString();
            }
        }

        if(_idMegnevIxs.code>-1){
            MetaField idc = _fields[_idMegnevIxs.code];
            MetaValue mv3 =  idc.GetMetaValue((char*)s);
            e.code = mv3.value.toString();
        }
        return e;
    }


    DataRowDefaultModel ToIdMegnevs(const QList<T>& data){
        DataRowDefaultModel e(_baseName);

        for(auto&a:data){
            IdMegnev i = ToIdMegnev(&a);
            e.AddValue(i);
        }

        return e;
    }

    // GUI_ADD
    MetaValue GetMetaValue(const T* s, const QString& fieldName){
        if(!s) return MetaValue("","",QMetaType(), RefType::None);
        //MetaField* field = T::Meta().GetMetaField(fieldName);
        MetaField* field = GetMetaField(fieldName);
        if(!field) return MetaValue("","",QMetaType(), RefType::None);
        MetaValue value = field->GetMetaValue((char*)s);
        return value;
    }

    QVariant GetValue(const T* s, const QString& fieldName){
        if(!s) return {};
        //MetaField* field = T::Meta().GetMetaField(fieldName);
        MetaField* field = GetMetaField(fieldName);
        if(!field) return {};
        QVariant value = field->GetValue((char*)s);
        return value;
    }

    // az AddMetaField helyességét ellenőrzi
    QString FlatSchemaDump()
    {
        QStringList fields;

        for(const auto& field : _fields)
        {
            fields.append(field.name);
        }

        return
            _baseName
            + "="
            + fields.join(';');
    }

};




