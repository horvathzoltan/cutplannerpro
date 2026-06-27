#pragma once
#include "materials/model/material_family.h"
#include <QMap>
#include <QVector>
#include <QUuid>
#include <product/model/material_role.h>

class MaterialRoleRegistry
{
public:
    void load(const QVector<MaterialRole>& roles);

    QVector<QString> prefixesFor(
        const QUuid& productTypeId,
        const QUuid& productSubtypeId,
        MaterialFamily family
        ) const;

private:
    // productTypeId → productSubtypeId → family → prefix-list
    // QMap<
    //     QUuid,
    //     QMap<
    //         QUuid,
    //         QMap<
    //             MaterialFamily,
    //             QVector<QString>
    //             >
    //         >
    //     > m_index;

    QVector<MaterialRole> m_roles;   // ⭐ egyszerű, átlátható
};
