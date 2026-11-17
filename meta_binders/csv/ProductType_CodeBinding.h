#pragma once

#include <product/registry/product_type_registry.h>


struct ProductType_CodeBinding
{
    using ValueType = QUuid;

    static QString toCsv(
        const QUuid& id)
    {
        const auto* p =
            ProductTypeRegistry::instance()
                .findById(id);

        if(!p)
            return {};

        return p->code;
    }

    static QUuid fromCsv(
        const QString& code)
    {
        const auto* p =
            ProductTypeRegistry::instance()
                .findByCode(code);

        if(!p)
            return {};

        return p->id;
    }
};