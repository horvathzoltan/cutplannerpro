#pragma once

#include "common/logger.h"
#include "ienumutils.h"

#include <QMetaType>

template<
    typename EnumType,
    typename Derived>
class EnumUtilsBase :
                      public IEnumUtils
{
public:

    std::type_index
    GetEnumType() const override
    {
        return
            std::type_index(
                typeid(EnumType));
    }

    void
    RegisterConverters()
        override
    {
        zInfo(
            QString("RegisterConverters %1")
                .arg(typeid(EnumType).name()));

        static bool done = false;

        if(done)
            return;

        done = true;

        QMetaType::registerConverter<
            EnumType,
            QString>(
            [](EnumType v)
            {
                return
                    Derived::toCsv(v);
            });

        QMetaType::registerConverter<
            QString,
            EnumType>(
            [](const QString& s)
            {
                return
                    Derived::fromCsv(s);
            });


        bool cc1 =
            QMetaType::canConvert(
                QMetaType::fromType<EnumType>(),
                QMetaType::fromType<QString>());

        zInfo(
            QString("enum->QString %1")
                .arg(cc1));


    }
};