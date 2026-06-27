#pragma once
#include "product/model/material_role.h"
#include <QVector>
#include <QString>

class ProductTypeRegistry;
class ProductSubtypeRegistry;

class MaterialRoleRepository
{
public:
    MaterialRoleRepository();

    QVector<MaterialRole> load(const QString& csvPath) const;
};
