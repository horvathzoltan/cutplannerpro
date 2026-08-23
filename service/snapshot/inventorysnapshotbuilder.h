#pragma once

#include "../../model/inventorysnapshot.h"
#include "leftover/registry/leftoverstockregistry.h"
#include "stock/registry/stockregistry.h"
#include "materials/registry/material_registry.h"
#include "materialbundles/registry/bundle_registry.h"
#include "common/logger.h"

/**
 * @brief Service osztály, amely InventorySnapshot-ot épít a registrykből.
 *
 * Ez a réteg választja le az optimizert az éles registrykről:
 * - az optimizer csak a snapshotot kapja,
 * - a registrykhez való hozzáférés itt történik.
 *
 * Nem végez validációt, nem mutat hibát, csak adatot ad vissza.
 * A validáció és a hibakezelés a presenter feladata.
 */

class InventorySnapshotBuilder {
public:
    /// Teljes készlet snapshot (stock + reusable)
    static InventorySnapshot build(int leftoverMinLength) {
        InventorySnapshot snapshot;
        snapshot.profileInventory = StockRegistry::instance().readAll();
        snapshot.reusableInventory = LeftoverStockRegistry::instance().filtered(leftoverMinLength);
        return snapshot;
    }

    static QMap<QUuid, int> greedyStrandPacking(
        const QMap<QUuid, QVector<int>>& lengthsPerMaterial)
    {
        QMap<QUuid, int> strandsPerMaterial;

        // végigmegyünk minden anyagon
        for (auto it = lengthsPerMaterial.begin(); it != lengthsPerMaterial.end(); ++it) {

            QUuid materialId = it.key();
            QVector<int> lengths = it.value();

            // ha nincs request erre az anyagra → 0 szál kell
            if (lengths.isEmpty()) {
                strandsPerMaterial[materialId] = 0;
                continue;
            }

            // szálhossz lekérése a MaterialMasterből
            const MaterialMaster* mm = MaterialRegistry::instance().findById(materialId);
            if (!mm) {
                // ismeretlen anyag → nem tudunk számolni
                strandsPerMaterial[materialId] = 0;
                continue;
            }

            int strandLength = mm->stockLength_mm;

            // csökkenő sorrend
            std::sort(lengths.begin(), lengths.end(), std::greater<int>());

            int strandCount = 0;
            int currentUsed = 0;

            for (int len : lengths) {
                if (currentUsed + len <= strandLength) {
                    currentUsed += len;
                } else {
                    strandCount++;
                    currentUsed = len;
                }
            }

            if (currentUsed > 0)
                strandCount++;

            strandsPerMaterial[materialId] = strandCount;
        }

        return strandsPerMaterial;
    }

    /// ÚJ: inventory snapshot a strand-igény alapján
    static InventorySnapshot build2(const QMap<QUuid, int>& strandsPerMaterial)
    {
        InventorySnapshot snapshot;

        auto appendUsedStrand = [&](const QUuid& matId, int qty, const QString& comment) {
            StockEntry used;
            used.materialId = matId;
            used.quantity   = qty;
            used.storageId  = QUuid();
            used.comment    = comment;
            snapshot.profileInventory.append(used);
        };

        // 0️⃣ Igényelt szálak
        zInfo("📌 Igényelt szálak:");
        for (auto it = strandsPerMaterial.begin(); it != strandsPerMaterial.end(); ++it) {
            const MaterialMaster* m = MaterialRegistry::instance().findById(it.key());
            if (!m) continue;
            zInfo(QString("• %1: %2 szál").arg(m->toReportLabel()).arg(it.value()));
        }

        // 1️⃣ Aggregált készlet (bundle robbantva → SIMPLE szálak)
        QMap<QUuid, int> aggregated = StockRegistry::instance().readAllAggregated();

        zInfo("📦 Aggregált szálkészlet:");
        for (auto it = aggregated.begin(); it != aggregated.end(); ++it) {
            const MaterialMaster* m = MaterialRegistry::instance().findById(it.key());
            QString name = m ? m->toReportLabel() : it.key().toString();
            zInfo(QString("   • %1 → %2 szál").arg(name).arg(it.value()));
        }

        // 2️⃣ Végigmegyünk a kért szálakon – itt MÁR SIMPLE szálak vannak robbantva
        for (auto it = strandsPerMaterial.begin(); it != strandsPerMaterial.end(); ++it) {

            QUuid materialId = it.key();
            int needStrands  = it.value();
            if (needStrands <= 0) continue;

            const MaterialMaster* master = MaterialRegistry::instance().findById(materialId);
            if (!master) {
                zWarning(QStringLiteral("⚠️ build2: Unknown materialId: %1").arg(materialId.toString()));
                continue;
            }

            // SIMPLE anyag
            if (master->kind == MaterialKind::Simple)
            {
                int haveStrands = aggregated.value(materialId, 0);
                if (haveStrands < needStrands) {
                    zWarning(QStringLiteral("⚠️ build2: Not enough strands for %1 (need: %2, have: %3)")
                                 .arg(master->toDisplay())
                                 .arg(needStrands)
                                 .arg(haveStrands));
                    continue;
                }

                // komponens levonása
                aggregated[materialId] -= needStrands;
                appendUsedStrand(materialId, needStrands, "Virtuális készlet");
                continue;
            }
            // BUNDLE anyag
            else if (master->kind == MaterialKind::Bundle)
            {

                // 1️⃣ komponens igény kiszámítása
                QMap<QUuid,int> needMap =
                    BundleRegistry::instance().computeComponentNeed(master->bundleCode, needStrands);

                bool allAvailable = true;

                // 2️⃣ komponensek ellenőrzése
                for (auto it2 = needMap.begin(); it2 != needMap.end(); ++it2) {
                    QUuid compId = it2.key();
                    int needPieces = it2.value();
                    int havePieces = aggregated.value(compId, 0);

                    if (havePieces < needPieces) {
                        const MaterialMaster* compMat = MaterialRegistry::instance().findById(compId);
                        QString compName = compMat ? compMat->toReportLabel() : compId.toString();

                        zWarning(QStringLiteral("⚠️ build2: Not enough component %1 for bundle %2 (need: %3, have: %4)")
                                     .arg(compName)
                                     .arg(master->bundleCode)
                                     .arg(needPieces)
                                     .arg(havePieces));

                        allAvailable = false;
                    }
                }

                if (!allAvailable)
                    continue;


                // 3️⃣ komponensek levonása
                for (auto it2 = needMap.begin(); it2 != needMap.end(); ++it2) {
                    aggregated[it2.key()] -= it2.value();
                }

                //snapshot.profileInventory.append(usedBundle);
                appendUsedStrand(materialId, needStrands, "Bundle – komponensekből összeállítva");
                continue;
            }
        }

        // 3️⃣ Leftover hozzáadása
        snapshot.reusableInventory =
            LeftoverStockRegistry::instance().filtered(300);

        // 4️⃣ Inventory logolása
        zInfo("📦 Inventoryba bekerült szálak:");
        if (snapshot.profileInventory.isEmpty()) {
            zInfo("• (nincs szál, hiány miatt)");
        } else {
            for (const auto& s : snapshot.profileInventory) {
                const MaterialMaster* m = MaterialRegistry::instance().findById(s.materialId);
                QString name = m ? m->toReportLabel() : s.materialId.toString();
                zInfo(QString("• %1 – %2 szál – tárhely: %3")
                          .arg(name)
                          .arg(s.quantity)
                          .arg(s.storageName()));
            }
        }

        return snapshot;
    }



};
