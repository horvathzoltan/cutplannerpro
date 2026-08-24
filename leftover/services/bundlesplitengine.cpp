#include "bundlesplitengine.h"
#include "leftover/registry/leftoverstockregistry.h"
#include "materials/registry/material_registry.h"
#include "materialbundles/registry/bundle_registry.h"
#include "common/logger.h"

#include <settings/settingsmanager.h>

#include <common/identifierutils.h>

static LeftoverStockEntry createLeftover(
    QUuid materialId,
    int length,
    QUuid storageId)
{
    LeftoverStockEntry e;
    e.materialId = materialId;
    e.availableLength_mm = length;
    e.storageId = storageId;
    e.source = Cutting::Result::LeftoverSource::Manual;

    int id = SettingsManager::instance().nextLeftoverCounter();
    auto barcode = IdentifierUtils::makeLeftoverId(id);

    e.barcode = barcode;
    e.status = LeftoverStatus::Good;
    return e;
}

BundleSplitResult BundleSplitEngine::applySplit(
    const LeftoverStockEntry& original,
    const QVector<BundleComponentLength>& remaining,
    const QVector<BundleComponentLength>& removed)
{
    BundleSplitResult out;

    // 1️⃣ Hiányos bundle → updatedOriginal
    out.updatedOriginal = original;

    // 🔧 PATCH: eredeti komponensek megtartása, hossz módosítása
    QMap<QUuid, int> newLengths;
    for (const auto& r : remaining)
        newLengths[r.materialId] = r.length_mm;

    QSet<QUuid> removedIds;
    for (const auto& r : removed)
        removedIds.insert(r.materialId);

    // új lista az eredeti alapján
    for (auto& comp : out.updatedOriginal.bundleComponentLengths)
    {
        if (removedIds.contains(comp.materialId))
        {
            comp.length_mm = 0;   // kivett komponens → nullázás
        }
        else if (newLengths.contains(comp.materialId))
        {
            comp.length_mm = newLengths[comp.materialId];  // maradó komponens → új hossz
        }
    }

    //out.updatedOriginal.bundleComponentLengths = remaining;

    // 🔧 PATCH: kivett komponensek nullázása a hiányos bundle-ben
    // QSet<QUuid> removedIds;
    // for (const auto& r : removed)
    //     removedIds.insert(r.materialId);

    // for (auto& comp : out.updatedOriginal.bundleComponentLengths)
    // {
    //     if (removedIds.contains(comp.materialId))
    //         comp.length_mm = 0;
    // }

    // 2️⃣ Kivett komponensek összegyűjtése anyagId szerint
    QMap<QUuid, QVector<int>> extracted;
    for (const auto& comp : removed)
        extracted[comp.materialId].append(comp.length_mm);

    // 3️⃣ Bundle definíciók lekérése
    QList<BundleDefinition> allBundles =
        BundleRegistry::instance().readAll();

    // 4️⃣ Minden anyagcsoportot feldolgozunk
    for (auto it = extracted.begin(); it != extracted.end(); ++it)
    {
        QUuid matId = it.key();
        QVector<int> lengths = it.value();

        const MaterialMaster* master =
            MaterialRegistry::instance().findById(matId);

        if (!master)
        {
            zWarning(QString("⚠️ BundleSplitEngine: material not found: %1")
                         .arg(matId.toString()));
            continue;
        }

        // 4/A: komponens-bundle felismerése (pl. CLB × 2 → CLB2)
        const BundleDefinition* singleBundle = nullptr;

        for (const auto& bd : allBundles)
        {
            if (bd.components.size() != 1)
                continue;

            const auto& comp = bd.components.first();
            if (comp.materialId == matId)
            {
                singleBundle = &bd;
                break;
            }
        }

        if (singleBundle)
        {
            int compCount = singleBundle->components.first().count;

            // hány bundle építhető?
            int buildCount = lengths.size() / compCount;

            const MaterialMaster* bundleMat =
                MaterialRegistry::instance().findByBundleCode(singleBundle->code);

            if (bundleMat)
            {
                for (int i = 0; i < buildCount; ++i)
                {
                    int len = lengths[i]; // minden komponens azonos hosszú
                    LeftoverStockEntry e =
                        createLeftover(bundleMat->id, len, original.storageId);

                    // 🔧 PATCH: bundle komponensek kitöltése (pl. CLB2 → két CLB komponens)
                    e.bundleComponentLengths.clear();
                    for (int j = 0; j < compCount; ++j)
                    {
                        BundleComponentLength bc;
                        bc.materialId = matId;
                        bc.length_mm = len;   // vagy -1, ha örökölni akarod
                        e.bundleComponentLengths.append(bc);
                    }

                    out.newLeftovers.append(e);

                }

                // felhasznált komponensek törlése
                lengths.erase(lengths.begin(), lengths.begin() + buildCount * compCount);
            }
        }

        // 4/B: maradék → natúr leftoverek
        for (int len : lengths)
        {
            LeftoverStockEntry e =
                createLeftover(matId, len, original.storageId);

            out.newLeftovers.append(e);
        }
    }

    return out;
}
