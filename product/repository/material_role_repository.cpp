#include "material_role_repository.h"

#include <QFile>
#include <QTextStream>
#include <QStringList>
#include <QDebug>

#include <materials/model/material_family_utils.h>

#include <product/registry/product_subtype_registry.h>
#include <product/registry/product_subtype_registry.h>
#include <product/registry/product_type_registry.h>

MaterialRoleRepository::MaterialRoleRepository(){}

QVector<MaterialRole> MaterialRoleRepository::load(const QString& csvPath) const
{
    QVector<MaterialRole> result;

    QFile f(csvPath);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "MaterialRoleRepository: cannot open" << csvPath;
        return result;
    }

    QTextStream in(&f);
    bool first = true;

    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (line.isEmpty())
            continue;

        // Skip header
        if (first) {
            first = false;
            continue;
        }

        QStringList cols = line.split(';');
        if (cols.size() < 4) {
            qWarning() << "MaterialRoleRepository: invalid row:" << line;
            continue;
        }

        QString typeCode    = cols[0].trimmed();
        QString subtypeCode = cols[1].trimmed();
        QString familyStr   = cols[2].trimmed();
        QString prefix      = cols[3].trimmed();

        const ProductType* type = ProductTypeRegistry::instance().findByCode(typeCode);
        if (!type) {
            qWarning() << "MaterialRoleRepository: unknown productType:" << typeCode;
            continue;
        }

        const ProductSubtype* subtype = ProductSubtypeRegistry::instance().findByCode(subtypeCode);
        if (!subtype) {
            qWarning() << "MaterialRoleRepository: unknown productSubtype:" << subtypeCode;
            continue;
        }

        MaterialFamily fam = MaterialFamilyUtils::fromString(familyStr);
        if (fam == MaterialFamily::Unknown) {
            qWarning() << "MaterialRoleRepository: unknown family:" << familyStr;
            continue;
        }

        MaterialRole r;
        r.productTypeId    = type->id;
        r.productSubtypeId = subtype->id;
        r.family           = fam;
        r.barcodePrefix    = prefix;

        result.append(r);
    }

    return result;
}


