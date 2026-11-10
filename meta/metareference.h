#pragma once

#include <typeindex>
#include <QList>
#include <QMetaType>
#include <QString>
#include <QVariant>
#include "common/logger.h"
#include "metavalue.h"

class RefBase{
public:
    RefBase(){}
protected:
    int _fieldIx;
    QString _refTypeName;
    int _refIx;
    std::type_index _a1 = std::type_index(typeid(void*));
    void* _metaInstancePtr;
    RefType _refType;

public:
    RefBase(int f, const QString& rt, int rf,RefType refType)
    {
        _fieldIx = f;
        _refTypeName = rt;
        _refIx = rf;
        _metaInstancePtr = nullptr;
        _refType = refType;
    }

    std::type_index a1(){return _a1;}
    QString refTypeName(){ return _refTypeName;}
    int fieldIx(){return _fieldIx;}
};

template<typename T>
class Ref : public RefBase
{
private:
    Ref(){}

public:

    Ref(int f, const QString& rt, int rf, RefType refType) : RefBase(f, rt, rf, refType)
    {
        _metaInstancePtr = &(T::Meta()._instance);//Address();
        _a1 = std::type_index(typeid(T));
    }

    T* metaInstance(){return reinterpret_cast<T*>(_metaInstancePtr);}
};

struct RefContainerKey
{
private:
    std::type_index _typeIndex = std::type_index(typeid(void*));
    int _fieldIndex = -1;

    RefContainerKey(){};

public:
    RefContainerKey(const std::type_index& a, int b)
    {
        _typeIndex = a;
        _fieldIndex = b;
    };

    bool
    operator<(const RefContainerKey& b) const{
        if(_typeIndex == b._typeIndex){
            return _fieldIndex < b._fieldIndex;
        }
        return _typeIndex<b._typeIndex;
    }

    QString name(){return _typeIndex.name();}
};

class RefContainer{
private:
    QMap<RefContainerKey, void*> _references;

    template<typename R>
    static RefContainerKey GetKey(int ix){
        RefContainerKey r(std::type_index(typeid(R)), ix);
        return r;
    }

public:
    template< typename R>
    void Add(int fIx, const QString& rt, const QString& rf, RefType refType){
        //int fIx = T::GetMetaFieldIx(f);
        int rIx = R::Meta().GetMetaFieldIx(rf);

        Ref<R>* r = new Ref<R>(fIx, rt, rIx, refType);
        RefContainerKey key = GetKey<Ref<R>>(fIx);
        _references.insert(key, r);

        zInfo("RefContainer added: "+QString(key.name()));
    }

    template<typename R>
    void Delete(int fIx){
        RefContainerKey key = GetKey<Ref<R>>(fIx);
        if(_references.contains(key)){
            void* b = _references.take(key);
            Ref<R>* e = reinterpret_cast<Ref<R>*>(b);
            delete e;

            zInfo("RefContainer deleted: "+QString(key.name()));
        } else{
            zWarning("RefContainer cannot delete: "+QString(key.name()));
        }
    }

    template<typename R>
    Ref<R>* Get(int fIx)
    {
        RefContainerKey key = GetKey<Ref<R>>(fIx);
        if(_references.contains(key))
        {
            void* b = _references.value(key);
            Ref<R>* e = reinterpret_cast<Ref<R>*>(b);
            zInfo("RefContainer find:" +e->refTypeName());
            return e;
        } else{
            zWarning("RefContainer cannnot found: "+QString(key.name()));
        }
        return nullptr;
    }
};

