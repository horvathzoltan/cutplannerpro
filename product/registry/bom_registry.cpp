#include "bom_registry.h"

#include <QHash>

BomRegistry& BomRegistry::instance() {
    static BomRegistry reg;
    return reg;
}

QHash<MaterialFamily, double>
BomRegistry::bomMap(const QUuid& typeId, const QUuid& subtypeId) const
{
    QHash<MaterialFamily, double> out;

    for (const auto& e : _data)
    {
        if (e.productTypeId == typeId &&
            e.productSubtypeId == subtypeId)
        {
            out[e.family] += e.quantity;
        }
    }

    return out;
}
