#include "material_role_registry.h"
#include "materials/model/material_family_detector.h"

#include <materials/model/material_family_utils.h>

void MaterialRoleRegistry::load(const QVector<MaterialRole>& roles)
{
    m_roles = roles;
}

MaterialRoleRegistry& MaterialRoleRegistry::instance()
{
    static MaterialRoleRegistry reg;
    return reg;
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

MaterialFamily MaterialRoleRegistry::familyForBarcode(const QString& barcode) const
{
    for (const auto& r : m_roles)
    {
        if (MaterialFamilyUtils::matchPrefix(barcode, r.barcodePrefix))
            return r.family;
    }

    return MaterialFamily::Unknown;
}

QVector<MaterialRole> MaterialRoleRegistry::findRoles(
    const QUuid& productTypeId,
    const QUuid& productSubtypeId
    ) const
{
    QVector<MaterialRole> result;

    for (const auto& r : m_roles)
    {
        if (r.productTypeId == productTypeId &&
            r.productSubtypeId == productSubtypeId)
        {
            result.append(r);
        }
    }

    return result;
}

QVector<MaterialRole> MaterialRoleRegistry::readAll() const
{
    return m_roles;
}

