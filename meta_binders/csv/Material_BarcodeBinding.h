#pragma once

#include "common/logger.h"
#include <materials/registry/material_registry.h>


struct Material_BarcodeBinding
{
    static const MaterialMaster*FindMaterial(const QUuid& id)
    {
        return MaterialRegistry::instance().findById(id);
    }

    static const MaterialMaster* FindMaterial(const QString& barcode)
    {
        return MaterialRegistry::instance().findByBarcode(barcode);
    }

    using ValueType =
        QUuid;

    static QString toCsv(
        const QUuid& materialId)
    {
        auto* material = FindMaterial(materialId);

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
        auto* material =FindMaterial(barcode);

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