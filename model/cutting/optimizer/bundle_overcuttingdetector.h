#pragma once

#include <QVector>
#include <QMap>
#include <QUuid>

#include <materials/registry/material_registry.h>

#include <leftover/registry/leftoverstockregistry.h>

#include "model/cutting/optimizer/optimizermodel.h"
#include "model/cutting/plan/request.h"
#include "materialbundles/registry/bundle_registry.h"
#include "materials/registry/material_registry.h"

/**
 * @brief Bundle komponensek túlvágásának detektálása.
 *
 * A CutPlan-eket vizsgálja:
 *  - minden bundle anyag esetén
 *  - minden levágott darabra rávetíti a bundle komponenseket
 *  - ha a darab hossza > komponens szálhossz → túlvágás
 *  - pótlólagos Cutting::Plan::Request-eket generál
 *
 * A detektor NEM módosítja az OptimizerModel-t.
 * A CuttingPresenter feladata az új requestek utó-optimalizálása.
 */
class BundleOverCuttingDetector
{
public:

    struct OvercutResult {
        QVector<Cutting::Plan::Request> newRequests;   // pótlólagos vágási igények
        QVector<Cutting::Plan::Request> newRequests_2;   // pótlólagos vágási igények
        bool hasOvercuts = false;                      // történt-e túlvágás

        // 🔗 bundleInstanceId → extra requestId lista
        QMap<QUuid, QVector<QUuid>> bundleMap;

        // determinisztikus sorrend miatt (audit)
        QString bundleMapToString() const {
            QStringList lines;

            QList<QUuid> keys = bundleMap.keys();
            std::sort(keys.begin(), keys.end(), [](const QUuid& a, const QUuid& b){
                return a.toString().localeAwareCompare(b.toString()) < 0;
            });

            for (const QUuid& key : keys) {
                QStringList reqIds;
                for (const QUuid& r : bundleMap[key]) {
                    reqIds << r.toString();
                }
                lines << QString("%1: [%2]").arg(key.toString(), reqIds.join(", "));
            }

            return lines.join("; ");
        }
    };

    static void logRequests(const QString& title,
                            const QVector<Cutting::Plan::Request>& reqs)
    {
        zInfo(title);
        if (reqs.isEmpty()) {
            zInfo("  • (nincs request)");
            return;
        }

        for (const auto& r : reqs)
        {
            const MaterialMaster* m =
                MaterialRegistry::instance().findById(r.materialId);

            QString matName = m ? m->toDisplay() : r.materialId.toString();

            zInfo(QString("  • %1 | len=%2 | qty=%3 | reqId=%4 | extRef=%5")
                      .arg(matName)
                      .arg(r.requiredLength)
                      .arg(r.quantity)
                      .arg(r.requestId.toString())
                      .arg(r.externalReference));
        }
    }

    /**
     * @brief bundle túlvágás detektálása az optimalizálás eredményén
     * @param model — az eredeti OptimizerModel (NEM módosítjuk!)
     * @return OvercutResult — új requestek listája
     */
    static OvercutResult detect(const Cutting::Optimizer::OptimizerModel& model)
    {
        OvercutResult result;

        const auto& plans = model.getResult_PlansRef();


        for (const auto& plan : plans)
        {
            QUuid matId = plan.materialId;
            const MaterialMaster* master = MaterialRegistry::instance().findById(matId);
            if (!master)
                continue;

            // ✔ csak bundle anyagok érdekesek
            if (master->kind != MaterialKind::Bundle)
                continue;

            auto bundleCode = master->bundleCode;
            auto components = BundleRegistry::instance().componentsOf(bundleCode);

            // 🔗 minden plan-ben lévő bundle darabhoz külön bundleInstanceId
            for (const auto& piece : plan.piecesWithMaterial)
            {
                QUuid bundleInstanceId = QUuid::createUuid();
                int pieceLen = piece.info.length_mm;

                for (const auto& comp : components)
                {
                    QUuid compMatId = comp.materialId;
                    const MaterialMaster* compMaster =
                        MaterialRegistry::instance().findById(compMatId);

                    if (!compMaster)
                        continue;

                    // ✔ helyes mező: stockLength_mm
                    int compStrandLen = static_cast<int>(compMaster->stockLength_mm);

                    // [PATCH] leftover-bundle túlvágás detektálása (CutPlan szintű lineage alapján)
                    std::optional<LeftoverStockEntry> loOpt;

                    if (plan.source == Cutting::Plan::Source::Reusable && !plan.sourceBarcode.isEmpty())
                    {
                        loOpt = LeftoverStockRegistry::instance().findByBarcode(plan.sourceBarcode);
                    }

                    if (loOpt.has_value())
                    {
                        const auto& lo = loOpt.value();

                        // csak akkor releváns, ha ugyanaz a bundle anyag
                        if (lo.materialId == master->id)
                        {
                            int loCompLen = lo.getComponentLength(compMatId);

                            // ha a leftover komponens rövidebb → az a szűk keresztmetszet
                            compStrandLen = std::min(compStrandLen, loCompLen);
                        }
                    }

                    // ✔ túlvágás detektálása
                    if (pieceLen > compStrandLen)
                    {
                        result.hasOvercuts = true;

                        // 1️⃣ eredeti request megkeresése
                        const Cutting::Plan::Request* originalReq = nullptr;

                        for (const auto& r : model.getRequests())   // <-- ez a helyes lista
                        {
                            if (r.requestId == piece.info.requestId)
                            {
                                originalReq = &r;
                                break;
                            }
                        }

                        Cutting::Plan::Request req;

                        if (originalReq)
                        {
                            // 2️⃣ teljes klónozás
                            req = *originalReq;

                            //req.requestId = QUuid::createUuid();
                            req.materialId = compMatId;

                        }
                        else
                        {
                            // fallback skeleton
                            req.requestId = QUuid::createUuid();
                            req.materialId = compMatId;
                        }

                        // if (originalReq)
                        // {
                        //     // 2️⃣ teljes klónozás
                        //     req = *originalReq;

                        //     // 🔧 MINDIG ÚJ requestId a pótló requesthez
                        //     req.requestId = QUuid::createUuid();
                        //     req.materialId = compMatId;
                        // }
                        // else
                        // {
                        //     // fallback skeleton
                        //     req.requestId = QUuid::createUuid();
                        //     req.materialId = compMatId;
                        // }

                        // 3️⃣ bundle‑komponens specifikus mezők átírása
                        req.requiredLength = pieceLen;
                        req.quantity = comp.count;

                        // 4️⃣ extRef öröklése + bundle‑jelölés
                        req.externalReference = piece.info.externalReference;
                            // originalReq && !originalReq->externalReference.isEmpty()
                            //     ? originalReq->externalReference
                            //     : QString("AUTO-BUNDLE-OVER-%1").arg(bundleCode);

                        // 5️⃣ hozzáadás
                        result.newRequests.append(req);

                        // 6️⃣ bundleInstanceId összekötés
                        result.bundleMap[bundleInstanceId].append(req.requestId);

                        zWarning(QStringLiteral(
                                     "⚠️ BundleOverCuttingDetector: túlvágás detektálva "
                                     "bundle=%1 komponens=%2 pieceLen=%3 strandLen=%4")
                                     .arg(master->toDisplay())
                                     .arg(compMaster->toDisplay())
                                     .arg(pieceLen)
                                     .arg(compStrandLen));
                    }

                }
            }
        }

        // 📋 RÉSZLETES ÖSSZEFOGLALÓ LOG A VÉGÉN
        if (!result.hasOvercuts) {
            zInfo("BundleOverCuttingDetector::detect — nincs detektált bundle túlvágás");
        } else {
            zInfo(QString("BundleOverCuttingDetector::detect — túlvágás összegzés: bundles=%1, newRequests=%2")
                      .arg(result.bundleMap.size())
                      .arg(result.newRequests.size()));

            // 🔍 Részletes request lista
            for (const auto& req : result.newRequests) {

                const MaterialMaster* mm = MaterialRegistry::instance().findById(req.materialId);
                QString matName = mm ? mm->toDisplay() : "?";

                zInfo(QString("  → Pótló request: material=%1, requiredLength=%2, quantity=%3, extRef=%4, requestId=%5")
                          .arg(matName)
                          .arg(req.requiredLength)
                          .arg(req.quantity)
                          .arg(req.externalReference)
                          .arg(req.requestId.toString()));
            }

            // 🔍 BundleInstance → RequestId lista
            for (auto it = result.bundleMap.constBegin(); it != result.bundleMap.constEnd(); ++it) {

                const QUuid& bundleInstanceId = it.key();
                const QList<QUuid>& reqIds = it.value();

                zInfo(QString("  • bundleInstanceId=%1 → %2 db pótló request")
                          .arg(bundleInstanceId.toString())
                          .arg(reqIds.size()));

                for (const auto& rid : reqIds) {
                    zInfo(QString("      ↳ requestId=%1").arg(rid.toString()));
                }
            }
        }


        return result;
    }

    // static const Cutting::Plan::Request* findRequest(
    //     const QVector<Cutting::Plan::Request>& all,
    //     const QUuid& id)
    // {
    //     for (const auto& r : all)
    //         if (r.requestId == id)
    //             return &r;
    //     return nullptr;
    // }

    static QVector<Cutting::Plan::Request>
    postProcessBundleOvercuts(const OvercutResult& result)
    {
        // Ha nincs túlvágás → eredeti requestek mennek tovább
        if (!result.hasOvercuts)
            return result.newRequests;

        QVector<Cutting::Plan::Request> out;

        // --- 0. Csoportosítás extRef szerint ---
        QMap<QString, QVector<Cutting::Plan::Request>> groups;
        for (const auto& r : result.newRequests)
            groups[r.externalReference].append(r);

        // --- 1. Bundle definíciók ---
        QList<BundleDefinition> allBundles =
            BundleRegistry::instance().readAll();

        // --- 2. Minden extRef‑csoportot külön dolgozunk fel ---
        for (auto it = groups.begin(); it != groups.end(); ++it)
        {
            const QString extRef = it.key();
            const auto& reqs     = it.value();

            // strandNeed csak erre az extRef‑re
            QMap<QUuid,int> remaining;
            for (const auto& r : reqs)
                remaining[r.materialId] += r.quantity;

            // --- 3/A Komponens‑bundle ---
            for (const auto& bd : allBundles)
            {
                if (bd.components.size() != 1)
                    continue;

                const auto& comp = bd.components.first();
                QUuid compMatId  = comp.materialId;
                int compCount    = comp.count;

                if (!remaining.contains(compMatId))
                    continue;

                int have = remaining[compMatId];
                if (have < compCount)
                    continue;

                int buildCount = have / compCount;

                // mintának az extRef‑hez tartozó eredeti request
                const Cutting::Plan::Request* base = nullptr;
                for (const auto& r : reqs)
                    if (r.materialId == compMatId)
                    { base = &r; break; }

                if (!base)
                    continue;

                const MaterialMaster* bundleMat =
                    MaterialRegistry::instance().findByBundleCode(bd.code);

                if (!bundleMat)
                    continue;

                for (int i = 0; i < buildCount; ++i)
                {
                    Cutting::Plan::Request req = *base;

                    // --- FIX: eredeti requestId megtartása ---
                    // req.requestId = QUuid::createUuid();   // törölve

                    req.materialId = bundleMat->id;
                    req.quantity   = 1;

                    // --- FIX: extRef nem módosul ---
                    req.externalReference = extRef;

                    out.append(req);
                }

                remaining[compMatId] -= buildCount * compCount;
            }

            // --- 3/B Teljes bundle ---
            for (const auto& bd : allBundles)
            {
                if (bd.components.size() <= 1)
                    continue;

                bool ok = true;
                int buildCount = INT_MAX;

                for (const auto& comp : bd.components)
                {
                    if (!remaining.contains(comp.materialId))
                    { ok = false; break; }

                    int have = remaining[comp.materialId];
                    int possible = have / comp.count;

                    if (possible == 0)
                    { ok = false; break; }

                    buildCount = std::min(buildCount, possible);
                }

                if (!ok || buildCount <= 0)
                    continue;

                const Cutting::Plan::Request* base = nullptr;
                for (const auto& r : reqs)
                    if (r.materialId == bd.components.first().materialId)
                    { base = &r; break; }

                if (!base)
                    continue;

                const MaterialMaster* bundleMat =
                    MaterialRegistry::instance().findByBundleCode(bd.code);

                if (!bundleMat)
                    continue;

                for (int i = 0; i < buildCount; ++i)
                {
                    Cutting::Plan::Request req = *base;

                    // --- FIX: eredeti requestId megtartása ---
                    // req.requestId = QUuid::createUuid();   // törölve

                    req.materialId = bundleMat->id;
                    req.quantity   = 1;

                    // --- FIX: extRef nem módosul ---
                    req.externalReference = extRef;

                    out.append(req);
                }

                for (const auto& comp : bd.components)
                    remaining[comp.materialId] -= buildCount * comp.count;
            }

            // --- 4. Maradék szálak ---
            for (auto rit = remaining.begin(); rit != remaining.end(); ++rit)
            {
                QUuid matId = rit.key();
                int qty     = rit.value();

                if (qty <= 0)
                    continue;

                const Cutting::Plan::Request* base = nullptr;
                for (const auto& r : reqs)
                    if (r.materialId == matId)
                    { base = &r; break; }

                Cutting::Plan::Request req;

                if (base)
                    req = *base;

                // --- FIX: eredeti requestId megtartása ---
                // req.requestId = QUuid::createUuid();   // törölve

                req.materialId = matId;
                req.quantity   = qty;
                req.externalReference = extRef;

                out.append(req);
            }
        }

        return out;
    }


    // static QVector<Cutting::Plan::Request>
    // postProcessBundleOvercuts(const OvercutResult& result)
    // {
    //     // Ha nincs túlvágás → eredeti requestek mennek tovább
    //     if (!result.hasOvercuts)
    //         return result.newRequests;

    //     QVector<Cutting::Plan::Request> out;

    //     // 1️⃣ Szál‑szintű igény összegyűjtése (materialId → totalQuantity)
    //     QMap<QUuid,int> strandNeed;
    //     for (const auto& r : result.newRequests)
    //         strandNeed[r.materialId] += r.quantity;

    //     // 2️⃣ Bundle definíciók lekérése
    //     QList<BundleDefinition> allBundles =
    //         BundleRegistry::instance().readAll();

    //     // 3️⃣ Bundle‑építés szál‑szinten
    //     QMap<QUuid,int> remaining = strandNeed;

    //     // 3/A ⭐ Komponens‑bundle felismerés (pl. NP‑CLB × 2 → NP‑CLB2 → BT‑CLB2)
    //     for (const auto& bd : allBundles)
    //     {
    //         // csak 1 komponensből álló bundle
    //         if (bd.components.size() != 1)
    //             continue;

    //         const auto& comp = bd.components.first();

    //         QUuid compMatId = comp.materialId;
    //         int compCount   = comp.count;

    //         if (!remaining.contains(compMatId))
    //             continue;

    //         int have = remaining[compMatId];
    //         if (have < compCount)
    //             continue;

    //         // ⭐ Építhető komponens‑bundle
    //         int buildCount = have / compCount;

    //         for (int i = 0; i < buildCount; ++i)
    //         {
    //             // 1️⃣ keresünk egy eredeti requestet mintának
    //             const Cutting::Plan::Request* base = nullptr;
    //             for (const auto& r : result.newRequests)
    //                 if (r.materialId == compMatId)
    //                 { base = &r; break; }

    //             if (!base)
    //                 continue;

    //             // 2️⃣ klónozzuk az eredeti pótló requestet
    //             Cutting::Plan::Request req = *base;

    //             // 3️⃣ új requestId
    //             req.requestId = QUuid::createUuid();

    //             // 4️⃣ bundle anyag lekérése
    //             const MaterialMaster* bundleMat =
    //                 MaterialRegistry::instance().findByBundleCode(bd.code);

    //             if (!bundleMat) {
    //                 zWarning(QString("⚠️ Bundle material not found for code %1").arg(bd.code));
    //                 continue;
    //             }

    //             // 5️⃣ anyag átírása → NP‑CLB2
    //             req.materialId = bundleMat->id;

    //             // 6️⃣ mennyiség átírása → 1 szál
    //             req.quantity = 1;

    //             // 7️⃣ extRef öröklése + bundle jelölés
    //             // req.externalReference =
    //             //     base->externalReference.isEmpty()
    //             //         ? QString("AUTO-COMPONENT-BUNDLE-%1").arg(bd.code)
    //             //         : base->externalReference;

    //             // extRef öröklése + bundle tag hozzáfűzése
    //             if (base->externalReference.isEmpty()) {
    //                 req.externalReference = QString("AUTO-%1-BUNDLE-%2")
    //                 .arg(bd.components.size() == 1 ? "COMPONENT" : "FULL")
    //                     .arg(bd.code);
    //             } else {
    //                 req.externalReference = base->externalReference +
    //                                         QString("|AUTO-%1-BUNDLE-%2")
    //                                             .arg(bd.components.size() == 1 ? "COMPONENT" : "FULL")
    //                                             .arg(bd.code);
    //             }

    //             // 8️⃣ hozzáadás
    //             out.append(req);
    //         }


    //         remaining[compMatId] -= buildCount * compCount;
    //     }

    //     // 3/B ⭐ Teljes bundle felismerés (pl. NP‑CL2+CLT2+CLB2)
    //     for (const auto& bd : allBundles)
    //     {
    //         if (bd.components.size() <= 1)
    //             continue;

    //         bool ok = true;
    //         int buildCount = INT_MAX;

    //         for (const auto& comp : bd.components)
    //         {
    //             if (!remaining.contains(comp.materialId))
    //             {
    //                 ok = false;
    //                 break;
    //             }

    //             int have = remaining[comp.materialId];
    //             int possible = have / comp.count;

    //             if (possible == 0)
    //             {
    //                 ok = false;
    //                 break;
    //             }

    //             buildCount = std::min(buildCount, possible);
    //         }

    //         if (!ok || buildCount <= 0)
    //             continue;

    //         // ⭐ Építhető teljes bundle
    //         for (int i = 0; i < buildCount; ++i)
    //         {
    //             // 1️⃣ keresünk mintának egy eredeti requestet
    //             const Cutting::Plan::Request* base = nullptr;
    //             for (const auto& r : result.newRequests)
    //                 if (r.materialId == bd.components.first().materialId)
    //                 { base = &r; break; }

    //             if (!base)
    //                 continue;

    //             // 2️⃣ klónozzuk az eredeti requestet
    //             Cutting::Plan::Request req = *base;

    //             // 3️⃣ új requestId
    //             req.requestId = QUuid::createUuid();

    //             // 4️⃣ bundle anyag lekérése
    //             const MaterialMaster* bundleMat =
    //                 MaterialRegistry::instance().findByBundleCode(bd.code);

    //             if (!bundleMat) {
    //                 zWarning(QString("⚠️ Bundle material not found for code %1").arg(bd.code));
    //                 continue;
    //             }

    //             // 5️⃣ anyag átírása → teljes bundle anyag
    //             req.materialId = bundleMat->id;

    //             // 6️⃣ mennyiség átírása → 1 szál
    //             req.quantity = 1;

    //             // 7️⃣ extRef öröklése + bundle jelölés
    //             // req.externalReference =
    //             //     base->externalReference.isEmpty()
    //             //         ? QString("AUTO-FULL-BUNDLE-%1").arg(bd.code)
    //             //         : base->externalReference;

    //             // extRef öröklése + bundle tag hozzáfűzése
    //             if (base->externalReference.isEmpty()) {
    //                 req.externalReference = QString("AUTO-%1-BUNDLE-%2")
    //                 .arg(bd.components.size() == 1 ? "COMPONENT" : "FULL")
    //                     .arg(bd.code);
    //             } else {
    //                 req.externalReference = base->externalReference +
    //                                         QString("|AUTO-%1-BUNDLE-%2")
    //                                             .arg(bd.components.size() == 1 ? "COMPONENT" : "FULL")
    //                                             .arg(bd.code);
    //             }

    //             // 8️⃣ hozzáadás
    //             out.append(req);
    //         }


    //         // komponensek levonása
    //         for (const auto& comp : bd.components)
    //             remaining[comp.materialId] -= buildCount * comp.count;
    //     }

    //     // 4️⃣ Maradék szálak → natúr requestek
    //     for (auto it = remaining.begin(); it != remaining.end(); ++it)
    //     {
    //         QUuid matId = it.key();
    //         int qty = it.value();

    //         if (qty <= 0)
    //             continue;

    //         Cutting::Plan::Request req;

    //         // keresünk mintának egy eredeti requestet
    //         const Cutting::Plan::Request* base = nullptr;
    //         for (const auto& r : result.newRequests)
    //             if (r.materialId == matId)
    //             { base = &r; break; }

    //         if (base)
    //             req = *base;

    //         req.requestId = QUuid::createUuid();
    //         req.materialId = matId;
    //         req.quantity = qty;
    //         req.externalReference = "AUTO-RAW-STRAND";

    //         out.append(req);
    //     }

    //     return out;
    // }

    // static QVector<Cutting::Plan::Request>
    // postProcessBundleOvercuts(const OvercutResult& result)
    // {
    //     // Ha nincs túlvágás → eredeti requestek mennek tovább
    //     if (!result.hasOvercuts)
    //         return result.newRequests;

    //     QVector<Cutting::Plan::Request> out;

    //     // 1️⃣ Szál‑szintű igény összegyűjtése (materialId → totalQuantity)
    //     QMap<QUuid,int> strandNeed;
    //     for (const auto& r : result.newRequests)
    //         strandNeed[r.materialId] += r.quantity;

    //     // 2️⃣ Bundle definíciók lekérése
    //     QList<BundleDefinition> allBundles =
    //         BundleRegistry::instance().readAll();

    //     // 3️⃣ Bundle‑építés szál‑szinten
    //     QMap<QUuid,int> remaining = strandNeed;

    //     // 3/A ⭐ Komponens‑bundle felismerés (pl. NP‑CLB × 2 → NP‑CLB2 → BT‑CLB2)
    //     for (const auto& bd : allBundles)
    //     {
    //         // csak 1 komponensből álló bundle
    //         if (bd.components.size() != 1)
    //             continue;

    //         const auto& comp = bd.components.first();

    //         QUuid compMatId = comp.materialId;
    //         int compCount   = comp.count;

    //         if (!remaining.contains(compMatId))
    //             continue;

    //         int have = remaining[compMatId];
    //         if (have < compCount)
    //             continue;

    //         // ⭐ Építhető komponens‑bundle
    //         int buildCount = have / compCount;

    //         for (int i = 0; i < buildCount; ++i)
    //         {
    //             Cutting::Plan::Request req;

    //             // keresünk egy eredeti requestet mintának
    //             const Cutting::Plan::Request* base = nullptr;
    //             for (const auto& r : result.newRequests)
    //                 if (r.materialId == compMatId)
    //                 { base = &r; break; }

    //             if (base)
    //                 req = *base;

    //             req.requestId = QUuid::createUuid();

    //             const MaterialMaster* bundleMat =
    //                 MaterialRegistry::instance().findByBundleCode(bd.code);

    //             if (!bundleMat) {
    //                 zWarning(QString("⚠️ Bundle material not found for code %1").arg(bd.code));
    //                 continue;
    //             }

    //             req.materialId = bundleMat->id;   // VALÓDI anyag ID

    //             req.quantity = 1;
    //             req.externalReference =
    //                 QString("AUTO-COMPONENT-BUNDLE-%1").arg(bd.code);

    //             out.append(req);
    //         }

    //         remaining[compMatId] -= buildCount * compCount;
    //     }

    //     // 3/B ⭐ Teljes bundle felismerés (pl. NP‑CL2+CLT2+CLB2)
    //     for (const auto& bd : allBundles)
    //     {
    //         if (bd.components.size() <= 1)
    //             continue;

    //         bool ok = true;
    //         int buildCount = INT_MAX;

    //         for (const auto& comp : bd.components)
    //         {
    //             if (!remaining.contains(comp.materialId))
    //             {
    //                 ok = false;
    //                 break;
    //             }

    //             int have = remaining[comp.materialId];
    //             int possible = have / comp.count;

    //             if (possible == 0)
    //             {
    //                 ok = false;
    //                 break;
    //             }

    //             buildCount = std::min(buildCount, possible);
    //         }

    //         if (!ok || buildCount <= 0)
    //             continue;

    //         // ⭐ Építhető teljes bundle
    //         for (int i = 0; i < buildCount; ++i)
    //         {
    //             Cutting::Plan::Request req;

    //             // keresünk mintának egy eredeti requestet
    //             const Cutting::Plan::Request* base = nullptr;
    //             for (const auto& r : result.newRequests)
    //                 if (r.materialId == bd.components.first().materialId)
    //                 { base = &r; break; }

    //             if (base)
    //                 req = *base;

    //             req.requestId = QUuid::createUuid();

    //             const MaterialMaster* bundleMat =
    //                 MaterialRegistry::instance().findByBundleCode(bd.code);

    //             if (!bundleMat) {
    //                 zWarning(QString("⚠️ Bundle material not found for code %1").arg(bd.code));
    //                 continue;
    //             }

    //             req.materialId = bundleMat->id;   // VALÓDI anyag ID

    //             req.quantity = 1;
    //             req.externalReference =
    //                 QString("AUTO-FULL-BUNDLE-%1").arg(bd.code);

    //             out.append(req);
    //         }

    //         // komponensek levonása
    //         for (const auto& comp : bd.components)
    //             remaining[comp.materialId] -= buildCount * comp.count;
    //     }

    //     // 4️⃣ Maradék szálak → natúr requestek
    //     for (auto it = remaining.begin(); it != remaining.end(); ++it)
    //     {
    //         QUuid matId = it.key();
    //         int qty = it.value();

    //         if (qty <= 0)
    //             continue;

    //         Cutting::Plan::Request req;

    //         // keresünk mintának egy eredeti requestet
    //         const Cutting::Plan::Request* base = nullptr;
    //         for (const auto& r : result.newRequests)
    //             if (r.materialId == matId)
    //             { base = &r; break; }

    //         if (base)
    //             req = *base;

    //         req.requestId = QUuid::createUuid();
    //         req.materialId = matId;
    //         req.quantity = qty;
    //         req.externalReference = "AUTO-RAW-STRAND";

    //         out.append(req);
    //     }

    //     return out;
    // }


    // static void postProcessBundleOvercuts(OvercutResult& result)
    // {
    //     if (!result.hasOvercuts)
    //         return;

    //     // 🔒 determinisztikus sorrend (audit)
    //     QList<QUuid> instanceIds = result.bundleMap.keys();
    //     std::sort(instanceIds.begin(), instanceIds.end(),
    //               [](const QUuid& a, const QUuid& b){
    //                   return a.toString().localeAwareCompare(b.toString()) < 0;
    //               });

    //     QVector<Cutting::Plan::Request> mergedRequests;

    //     for (const QUuid& instanceId : instanceIds)
    //     {
    //         const QVector<QUuid>& reqIds = result.bundleMap[instanceId];

    //         // 🔍 összegyűjtjük a darab-szintű requesteket
    //         QVector<Cutting::Plan::Request> group;
    //         group.reserve(reqIds.size());

    //         for (const QUuid& rid : reqIds)
    //         {
    //             for (const auto& r : result.newRequests)
    //             {
    //                 if (r.requestId == rid)
    //                 {
    //                     group.append(r);
    //                     break;
    //                 }
    //             }
    //         }

    //         if (group.isEmpty())
    //             continue;

    //         // 🔍 komponensek anyagId szerint csoportosítva
    //         QMap<QUuid, int> compCount;
    //         for (const auto& r : group)
    //             compCount[r.materialId] += r.quantity;

    //         // 🔍 bundle anyag meghatározása
    //         // (minden darab-szintű request ugyanahhoz a bundle-hez tartozik)
    //         QUuid bundleMatId;
    //         {
    //             // az első komponens anyagából visszafejtjük a bundle-t
    //             const MaterialMaster* compMaster =
    //                 MaterialRegistry::instance().findById(group.first().materialId);
    //             if (!compMaster)
    //                 continue;

    //             QString bundleCode = compMaster->bundleCode;


    //             const BundleDefinition* bundleDef =
    //                 BundleRegistry::instance().findByCode(bundleCode);

    //             if (!bundleDef)
    //                 continue;

    //             bundleMatId = bundleDef->id;

    //         }

    //         // 🔍 bundle komponensek lekérése
    //         auto bundleComponents =
    //             BundleRegistry::instance().componentsOf(
    //                 MaterialRegistry::instance().findById(bundleMatId)->bundleCode);

    //         // 🔍 ellenőrizzük, hogy minden komponens megvan-e
    //         bool fullBundle = true;
    //         for (const auto& bc : bundleComponents)
    //         {
    //             if (!compCount.contains(bc.materialId))
    //             {
    //                 fullBundle = false;
    //                 break;
    //             }
    //         }

    //         if (fullBundle)
    //         {
    //             // ⭐ teljes bundle pótlás
    //             Cutting::Plan::Request bundleReq;
    //             bundleReq.requestId = QUuid::createUuid();
    //             bundleReq.materialId = bundleMatId;

    //             // a darab-szintű requestek requiredLength-je azonos
    //             bundleReq.requiredLength = group.first().requiredLength;
    //             bundleReq.quantity = 1;

    //             bundleReq.externalReference =
    //                 QString("AUTO-BUNDLE-FULL-%1")
    //                     .arg(MaterialRegistry::instance()
    //                              .findById(bundleMatId)->bundleCode);

    //             mergedRequests.append(bundleReq);
    //         }
    //         else
    //         {
    //             // ⭐ fél-bundle pótlás (pl. 2 betét → BT-CLB2)
    //             for (auto it = compCount.begin(); it != compCount.end(); ++it)
    //             {
    //                 QUuid compMatId = it.key();
    //                 int count = it.value();

    //                 if (count <= 1)
    //                     continue;

    //                 Cutting::Plan::Request halfReq;
    //                 halfReq.requestId = QUuid::createUuid();
    //                 halfReq.materialId = compMatId;
    //                 halfReq.requiredLength = group.first().requiredLength;
    //                 halfReq.quantity = count;

    //                 halfReq.externalReference =
    //                     QString("AUTO-BUNDLE-HALF-%1x%2")
    //                         .arg(MaterialRegistry::instance()
    //                                  .findById(compMatId)->toDisplay())
    //                         .arg(count);

    //                 mergedRequests.append(halfReq);
    //             }
    //         }
    //     }

    //     // 🔄 felülírjuk a darab-szintű requesteket a bundle-szintűekkel
    //     result.newRequests = mergedRequests;
    // }

};
