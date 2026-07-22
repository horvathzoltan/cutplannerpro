#include "bom_registry.h"

#include "material_role_registry.h"
#include "product/material_role_utils.h"

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

QMap<QString, double> BomRegistry::bomRoleMap(const QUuid& typeId, const QUuid& subtypeId) const
{
    QMap<QString, double> out;

    auto famMap = bomMap(typeId, subtypeId);   // family → qty
    auto roles  = MaterialRoleRegistry::instance().findRoles(typeId, subtypeId);

    for (const auto& role : roles)
    {
        // A család BOM mennyisége
        double familyQty = famMap.value(role.family, 0.0);

        // A role BOM mennyisége mindig 1 (workaround)
        // Mert a láb komponensek (CL, CLB, CLT) együtt alkotnak 1 lábpárt.
        double roleQty = (familyQty > 0 ? 1.0 : 0.0);

        QString normalized = MaterialRoleUtils::normalizePrefix(role.barcodePrefix);
        out[normalized] = roleQty;

        //out[role.barcodePrefix] = roleQty;
    }

    return out;
}
