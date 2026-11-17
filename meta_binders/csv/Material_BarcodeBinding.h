#pragma once

#include "common/logger.h"
#include <materials/registry/material_registry.h>


struct Material_BarcodeBinding
{
    using ValueType = QUuid;

    static QString toCsv(
        const QUuid& materialId)
    {
        auto* material = MaterialRegistry::instance().findById(materialId);

        if(!material)
        {
            zWarning(
                QString(
                    "Material not found: %1")
                    .arg(
                        materialId.toString()));

            return {};
        }

        return material->barcode;
    }

    static QUuid fromCsv(
        const QString& barcode)
    {
        auto* material = MaterialRegistry::instance().findByBarcode(barcode);

        if(!material)
        {
            zWarning(
                QString(
                    "Material barcode not found: %1")
                    .arg(
                        barcode));

            return {};
        }

        return material->id;
    }
};