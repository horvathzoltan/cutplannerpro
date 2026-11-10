#pragma once

#include <QString>
#include <QVariant>

enum class RefType{
    None, R_1N, R_NM
};

struct MetaValue{
private:
    MetaValue(){};
public:
    MetaValue(const QString& _name, const QString& _wcode,const QMetaType& _type, RefType _refType)
    {
        metaField_name = _name;
        wcode = _wcode;
        value = QVariant(_type);
        type = _type;
        refType = _refType;
    }

    MetaValue(const QString& _name, const QString& _wcode,const QVariant& _value, RefType _refType)
    {
        metaField_name = _name;
        wcode = _wcode;
        value = _value;
        type = _value.metaType();
        refType = _refType;
    }

    // ez köti a MetaField-hez, talán jobb lenne egy index
    // név vélkül viszont nem tudjuk megnevezni hibaüzenetben
    QString metaField_name;
    //int metaField_ix;

    QString wcode;
    QString translatedName;

    QVariant value;
    QMetaType type;

    RefType refType;

    bool isValid(){ return !metaField_name.isEmpty();}
};

struct MetaValidationMessage{
private:
    QString _name;
    QString _wcode;
    // QString _value;
    QString _logMessage;

public:
    MetaValidationMessage(const QString& name, const QString& wcode, const QString& logMessage)
    {
        _name = name;
        _wcode = wcode;
        _logMessage = logMessage;
    }

    QString logMessage(){return _logMessage;}
    QString name(){return _name;}
    QString wcode(){return _wcode;}

};
