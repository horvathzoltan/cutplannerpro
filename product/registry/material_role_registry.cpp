#include "material_role_registry.h"
#include "materials/model/material_family_detector.h"

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
        QString px = r.barcodePrefix.trimmed();

        // ha csillag van a végén, levágjuk (emberi jelölés)
        if (px.endsWith("*"))
            px.chop(1);

        if (matchPrefix(barcode, px))
            return r.family;
    }

    return MaterialFamily::Unknown;
}

