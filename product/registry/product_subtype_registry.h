#pragma once

#include "product/model/product_subtype.h"
#include <QVector>
#include <QString>
#include <QUuid>

class ProductSubtypeRegistry {
private:
    ProductSubtypeRegistry() = default;
    QVector<ProductSubtype> _data;

public:
    static ProductSubtypeRegistry& instance();

    void setData(const QVector<ProductSubtype>& v) { _data = v; }

    const QVector<ProductSubtype>& readAll() const { return _data; }

    QVector<ProductSubtype> findByTypeId(const QUuid& typeId) const {
        QVector<ProductSubtype> out;
        for (const auto& s : _data)
            if (s.typeId == typeId)
                out.append(s);
        return out;
    }

    // const ProductSubtype* findByName(const QString& name) const {
    //     for (const auto& s : _data)
    //         if (s.name.compare(name, Qt::CaseInsensitive) == 0)
    //             return &s;
    //     return nullptr;
    // }

    const ProductSubtype* findByCode(const QString& code) const {
        for (const auto& s : _data)
            if (s.code.compare(code, Qt::CaseInsensitive) == 0)
                return &s;
        return nullptr;
    }

};
