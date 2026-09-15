#include "material_role_registry.h"
#include "materials/model/material_master.h"
#include "materials/model/material_rolegroup.h"
#include <materials/model/material_family_utils.h>
#include <materials/registry/material_registry.h>
#include <materials/registry/material_rolegroup_registry.h>

void MaterialRoleRegistry::load(const QVector<MaterialRole>& roles)
{
    m_roles = roles;
}

MaterialRoleRegistry& MaterialRoleRegistry::instance()
{
    static MaterialRoleRegistry reg;
    return reg;
}


// QVector<QString> MaterialRoleRegistry::prefixesFor(
//     const QUuid& productTypeId,
//     const QUuid& productSubtypeId,
//     MaterialFamily family
//     ) const
// {
//     QVector<QString> result;

//     for (const auto& r : m_roles)
//     {
//         if (r.productTypeId == productTypeId &&
//             r.productSubtypeId == productSubtypeId &&
//             r.family == family)
//         {
//             result.append(r.barcodePrefix);
//         }
//     }

//     return result;
// }

// MaterialFamily MaterialRoleRegistry::familyForBarcode(const QString& barcode) const
// {
//     for (const auto& r : m_roles)
//     {
//         if (MaterialFamilyUtils::matchPrefix(barcode, r.barcodePrefix))
//             return r.family;
//     }

//     return MaterialFamily::Unknown;
// }

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

QVector<MaterialRole> MaterialRoleRegistry::findRoles(
    const QUuid& productTypeId,
    const QUuid& productSubtypeId,
    const MaterialFamily& family
    ) const
{
    QVector<MaterialRole> result;

    for (const auto& r : m_roles)
    {
        if (r.productTypeId == productTypeId &&
            r.productSubtypeId == productSubtypeId &&
            r.family == family)
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

MaterialRole MaterialRoleRegistry::roleForBarcode(const QString& barcode) const
{
    // 1) Anyag feloldása barcode alapján
    const MaterialMaster* mat = MaterialRegistry::instance().findByBarcode(barcode);
    if (!mat) {
        return MaterialRole{}; // Unknown
    }

    // 2) Szerepkör-csoport feloldása anyag alapján
    const MaterialRoleGroup* rg =
        MaterialRoleGroupRegistry::instance().findByMaterialId(mat->id);

    if (!rg) {
        return MaterialRole{}; // Unknown
    }

    // 3) Szerepkör keresése groupId alapján
    for (const auto& r : m_roles)
    {
        if (r.groupId == rg->id)
            return r;
    }

    // 4) Ha nincs szerepkör → Unknown
    return MaterialRole{};
}



