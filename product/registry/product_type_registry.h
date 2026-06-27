#pragma once

#include "product/model/product_type.h"
#include <QVector>
#include <QString>
#include <QUuid>

class ProductTypeRegistry {
private:
    ProductTypeRegistry() = default;
    QVector<ProductType> _data;

public:
    static ProductTypeRegistry& instance();

    void setData(const QVector<ProductType>& v) { _data = v; }

    const QVector<ProductType>& readAll() const { return _data; }

    const ProductType* findById(const QUuid& id) const {
        for (const auto& t : _data)
            if (t.id == id)
                return &t;
        return nullptr;
    }

//     const ProductType* findByName(const QString& name) const {
//         for (const auto& t : _data)
//             if (t.name.compare(name, Qt::CaseInsensitive) == 0)
//                 return &t;
//         return nullptr;
//     }

    const ProductType* findByCode(const QString& code) const {
        for (const auto& t : _data)
            if (t.code.compare(code, Qt::CaseInsensitive) == 0)
                return &t;
        return nullptr;
    }

};
