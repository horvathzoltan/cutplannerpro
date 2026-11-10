#include "rbcsv_valueformatter.h"

#include <QByteArray>
#include <QDate>
#include <QDateTime>
#include <QUuid>

#include "meta/metafield.h"

QString RbCsvValueFormatter::ToString(
    const MetaField* field,
    const QVariant& value)
{
    Q_UNUSED(field)

    if(!value.isValid())
        return {};

    switch(value.metaType().id())
    {
    case QMetaType::QString:
        return value.toString();

    case QMetaType::QByteArray:
        return value
            .toByteArray()
            .toBase64();

    case QMetaType::QDate:
        return value
            .toDate()
            .toString(Qt::ISODate);

    case QMetaType::QDateTime:
        return value
            .toDateTime()
            .toString(Qt::ISODate);

    case QMetaType::QUuid:
        return value
            .toUuid()
            .toString(QUuid::WithoutBraces);

    default:
        return value.toString();
    }
}