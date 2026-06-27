// #pragma once
// #include <QVector>
// #include <QString>
// #include <QUuid>

// #include "product/registry/material_role_registry.h"
// #include "product/registry/bom_registry.h"
// #include "materials/registry/material_registry.h"
// #include "materials/model/material_master.h"
// #include "materials/model/material_family.h"

// class MaterialSelector
// {
// public:
//     struct Result {
//         const MaterialMaster* material = nullptr;
//         QString reason;
//     };

//     static Result select(
//         const QUuid& productTypeId,
//         const QUuid& productSubtypeId,
//         MaterialFamily family,
//         const QString& desiredColor,
//         double requiredLength
//         );
// };
