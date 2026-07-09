#pragma once
#include "materials/model/material_family.h"
#include <QMap>
#include <QVector>
#include <QUuid>
#include <product/model/material_role.h>

class MaterialRoleRegistry
{
private:
    MaterialRoleRegistry() = default;   // ⭐ singleton ctor
    QVector<MaterialRole> m_roles;

public:
    static MaterialRoleRegistry& instance();   // ⭐ singleton accessor

    QVector<MaterialRole> readAll() const;

    void load(const QVector<MaterialRole>& roles);

    QVector<QString> prefixesFor(
        const QUuid& productTypeId,
        const QUuid& productSubtypeId,
        MaterialFamily family
        ) const;

    MaterialFamily familyForBarcode(const QString& barcode) const;

    QVector<MaterialRole> findRoles(
        const QUuid& productTypeId,
        const QUuid& productSubtypeId
        ) const;

};
