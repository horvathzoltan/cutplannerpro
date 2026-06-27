#include "material_role_registry.h"

void MaterialRoleRegistry::load(const QVector<MaterialRole>& roles)
{
    m_roles = roles;
}

QVector<QString> MaterialRoleRegistry::prefixesFor(
    const QUuid& productTypeId,
    const QUuid& productSubtypeId,
    MaterialFamily family
    ) const
{
    QVector<QString> result;

    for (const auto& r : m_roles)
    {
        if (r.productTypeId == productTypeId &&
            r.productSubtypeId == productSubtypeId &&
            r.family == family)
        {
            result.append(r.barcodePrefix);
        }
    }

    return result;
}
