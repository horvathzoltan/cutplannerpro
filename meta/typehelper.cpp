#include "typehelper.h"
#include "common/logger.h"
#include <QMetaType>

// template<typename T>
// std::type_index TypeHelper::GetIndex()
// {
//     return std::type_index(typeid(T));
// }

int TypeHelper::ToNullable(int typeId)
{
    if(typeId == QMetaType::Type::Bool) return qMetaTypeId<std::optional<bool>>();

    if(typeId == QMetaType::Type::LongLong) return qMetaTypeId<std::optional<qint64>>();
    if(typeId == QMetaType::Type::Int) return qMetaTypeId<std::optional<qint32>>();
    if(typeId == QMetaType::Type::Short) return qMetaTypeId<std::optional<qint16>>();
    if(typeId == QMetaType::Type::Char) return qMetaTypeId<std::optional<qint8>>();

    if(typeId == QMetaType::Type::ULongLong) return qMetaTypeId<std::optional<quint64>>();
    if(typeId == QMetaType::Type::UInt) return qMetaTypeId<std::optional<quint32>>();
    if(typeId == QMetaType::Type::UShort) return qMetaTypeId<std::optional<quint16>>();
    if(typeId == QMetaType::Type::UChar) return qMetaTypeId<std::optional<quint8>>();

    if(typeId == QMetaType::Type::Double) return qMetaTypeId<std::optional<double>>();
    if(typeId == QMetaType::Type::Float) return qMetaTypeId<std::optional<float>>();

    if(typeId == QMetaType::Type::QString) return QMetaType::Type::QString;
    if(typeId == QMetaType::Type::QDateTime) return QMetaType::Type::QDateTime;

    QString msg = "ToNullable not implements conversion to typeId: "+QString::number(typeId);

    QMetaType t = QMetaType(typeId);
    if(t.isValid())
    {
        msg += QStringLiteral(" ")+t.name();
    }
    zWarning(msg);
    return -1;
}