#pragma once

#include <product/registry/product_subtype_registry.h>

struct ProductSubType_CodeBinding
{
    using ValueType = QUuid;

    static QString toCsv(
        const QUuid& id)
    {
        const auto* p =
            ProductSubtypeRegistry::instance()
                .findById(id);

        if(!p)
            return {};

        return p->code;
    }

    static QUuid fromCsv(
        const QString& code)
    {
        const auto* p =
            ProductSubtypeRegistry::instance()
                .findByCode(code);

        if(!p)
            return {};

        return p->id;
    }
};