#pragma once

#include "model/cutting/plan/request.h"
#include <QStringList>
#include <leftover/registry/leftoverstockregistry.h>
#include <materials/registry/material_group_registry.h>
#include <materials/registry/material_registry.h>
#include <stock/registry/stockregistry.h>
#include "common/validation_result.h"

namespace CuttingPlanValidationService {

inline ValidationResult validate(
    const QVector<Cutting::Plan::Request>& requests)
{
    ValidationResult result;

    // 0️⃣ Alapvető adatforrások ellenőrzése
    if (MaterialRegistry::instance().isEmpty())
        result.errors << "Az anyagtörzs üres vagy nem sikerült betölteni.";

    if (MaterialGroupRegistry::instance().isEmpty())
        result.warnings << "Az anyagcsoportok üresek – a helyettesítés nem fog működni.";

    if (StockRegistry::instance().isEmpty())
        result.warnings << "A készlet üres – csak hullókból vagy snapshotból lehet vágni.";

    if (LeftoverStockRegistry::instance().isEmpty())
        result.warnings << "A hullókészlet üres – toldás nem fog működni.";

    // 1️⃣ Request lista beolvasása
    if (requests.isEmpty())
        result.errors << "Nincs megadva vágási igény.";

    // 2️⃣ Request anyagok validálása
    for (const auto& r : requests) {
        const MaterialMaster* mm = MaterialRegistry::instance().findById(r.materialId);
        if (!mm) {
            result.errors << QString("Ismeretlen anyag a vágási igényben: %1")
                                 .arg(r.materialId.toString());
            continue;
        }

        if (mm->stockLength_mm <= 0)
            result.errors << QString("Az anyaghoz nincs érvényes szálhossz megadva: %1")
                                 .arg(mm->toDisplay());

    }

    // 3️⃣ Ha bármelyik input hiba kritikus → megállunk
    return result;
}

} //endof