#include "materials/registry/material_registry.h"

#include <product/registry/bom_registry.h>
#include <product/registry/material_role_registry.h>
#include <QHash>

#include <materials/model/material_family_utils.h>

MaterialRegistry &MaterialRegistry::instance() {
    static MaterialRegistry reg;
    return reg;
}

const MaterialMaster* MaterialRegistry::findById(const QUuid& id) const {
    for (const auto& m : _data)
        if (m.id == id)
            return &m;
    return nullptr;
}

const MaterialMaster* MaterialRegistry::findByBarcode(const QString& barcode) const {
    for (const auto& m : _data)
        if (m.barcode == barcode)
            return &m;
    return nullptr;
}

bool MaterialRegistry::isBarcodeUnique(const QString& barcode) const {
    for (const auto& m : _data)
        if (m.barcode == barcode)
            return false;
    return true;
}

bool MaterialRegistry::registerData(const MaterialMaster& material) {
    if (!isBarcodeUnique(material.barcode))
        return false;
    _data.append(material);
    return true;
}

QVector<QUuid> MaterialRegistry::generateBom(
    QUuid typeId,
    QUuid subtypeId) const
{
    QVector<QUuid> ordered;

    // 1) BOM családok
    QHash<MaterialFamily, double> bomFamilies =
        BomRegistry::instance().bomMap(typeId, subtypeId);

    // 2) Rolemap prefixek
    QVector<MaterialRole> roles =
        MaterialRoleRegistry::instance().findRoles(typeId, subtypeId);

    // 3) BOM sorrend (deterministic)
    QList<MaterialFamily> famOrder;
    for (const auto& e : BomRegistry::instance().readAll()) {
        if (e.productTypeId == typeId &&
            e.productSubtypeId == subtypeId)
        {
            famOrder << e.family;
        }
    }

    // 4) Anyagok stabil sorrendben
    auto mats = readAll();
    std::sort(mats.begin(), mats.end(),
              [](const MaterialMaster& a, const MaterialMaster& b) {
                  return a.barcode < b.barcode;
              });

    // 5) Családonként teljes anyaglista
    for (MaterialFamily fam : famOrder) {

        // prefixek gyűjtése (csillag marad!)
        QStringList famPrefixes;
        for (const auto& role : roles) {
            if (role.family == fam) {
                famPrefixes << role.barcodePrefix.trimmed();
            }
        }
        famPrefixes.sort();

        // anyagok gyűjtése prefix + wildcard alapján
        for (const auto& prefix : famPrefixes) {
            for (const auto& mat : mats) {

                if (mat.family != fam)
                    continue;

                if (!MaterialFamilyUtils::matchPrefix(mat.barcode, prefix))
                    continue;

                ordered << mat.id;
            }
        }
    }

    return ordered;
}

