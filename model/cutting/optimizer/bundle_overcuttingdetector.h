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

    // NEW: bundle rod consumption tracker
    struct BundleRodState {
        // componentMaterialId → per-strand remaining lengths (mm)
        QHash<QUuid, QVector<int>> remainingStrands;
    };


    // NEW: initialize bundle rod state from stock or leftover
    static BundleRodState initBundleRodState(
        const MaterialMaster* bundleMaster,
        const QString& sourceBarcode)
    {
        BundleRodState st;

        auto comps = BundleRegistry::instance().componentsOf(bundleMaster->bundleCode);

        // leftover override
        std::optional<LeftoverStockEntry> loOpt;
        if (!sourceBarcode.isEmpty())
            loOpt = LeftoverStockRegistry::instance().findByBarcode(sourceBarcode);

        for (const auto& c : comps)
        {
            const MaterialMaster* cm = MaterialRegistry::instance().findById(c.materialId);
            if (!cm)
                continue;

            int baseLen = static_cast<int>(cm->rawStockLength_mm());

            if (loOpt.has_value() && loOpt->materialId == bundleMaster->id) {
                int loLen = loOpt->getComponentLength(c.materialId);
                baseLen = std::min(baseLen, loLen);
            }

            QVector<int> strands;
            strands.reserve(c.count);

            for (int i = 0; i < c.count; ++i)
                strands.append(baseLen);

            st.remainingStrands[c.materialId] = strands;
        }

        return st;
    }




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

            // 🔢 plan-szintű bundle rod state (egy rúd = egy plan)
            BundleRodState state = initBundleRodState(
                master,
                (plan.source == Cutting::Plan::Source::Reusable) ? plan.sourceBarcode : QString());

            // 🔗 bundleInstanceId: egy plan-hez egy instance (egy rúd)
            QUuid bundleInstanceId = QUuid::createUuid();

            // 🔗 minden plan-ben lévő bundle darab
            for (const auto& piece : plan.piecesWithMaterial)
            {
                int pieceLen = piece.info.length_mm;

                auto components = BundleRegistry::instance().componentsOf(master->bundleCode);

                for (const auto& comp : components)
                {
                    QUuid compMatId = comp.materialId;
                    const MaterialMaster* compMaster =
                        MaterialRegistry::instance().findById(compMatId);

                    if (!compMaster)
                        continue;

                    auto itStrands = state.remainingStrands.find(compMatId);
                    if (itStrands == state.remainingStrands.end())
                        continue;

                    QVector<int>& strands = itStrands.value();

                    // strand-szintű túlvágás detektálása:
                    // ha bármelyik szál nem adja ki a darabot → overcut
                    bool overcutHere = false;

                    for (int i = 0; i < strands.size(); ++i)
                    {
                        if (strands[i] < pieceLen) {
                            overcutHere = true;
                            break;
                        }
                    }

                    // ha bármelyik szál nem adja ki → túlvágás
                    if (overcutHere)
                    {
                        result.hasOvercuts = true;

                        const Cutting::Plan::Request* originalReq = nullptr;

                        for (const auto& r : model.getRequests())
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
                            req = *originalReq;
                            req.materialId = compMatId;
                        }
                        else
                        {
                            req.requestId = QUuid::createUuid();
                            req.materialId = compMatId;
                        }

                        req.requiredLength = pieceLen;
                        req.quantity       = comp.count;
                        req.externalReference = piece.info.externalReference;

                        result.newRequests.append(req);
                        result.bundleMap[bundleInstanceId].append(req.requestId);

                        zWarning(QStringLiteral(
                                     "⚠️ BundleOverCuttingDetector: túlvágás detektálva (strand-szintű) "
                                     "bundle=%1 komponens=%2 pieceLen=%3")
                                     .arg(master->toDisplay())
                                     .arg(compMaster->toDisplay())
                                     .arg(pieceLen));
                    }
                    else
                    {
                        // minden szál kiadja → mindegyikből levonjuk a pieceLen-t
                        for (int i = 0; i < strands.size(); ++i)
                            strands[i] -= pieceLen;
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

    /**
     * @brief bundle túlvágás detektálása az optimalizálás eredményén
     * @param model — az eredeti OptimizerModel (NEM módosítjuk!)
     * @return OvercutResult — új requestek listája
     */
    // static OvercutResult detect(const Cutting::Optimizer::OptimizerModel& model)
    // {
    //     OvercutResult result;

    //     const auto& plans = model.getResult_PlansRef();


    //     for (const auto& plan : plans)
    //     {
    //         QUuid matId = plan.materialId;
    //         const MaterialMaster* master = MaterialRegistry::instance().findById(matId);
    //         if (!master)
    //             continue;

    //         // ✔ csak bundle anyagok érdekesek
    //         if (master->kind != MaterialKind::Bundle)
    //             continue;

    //         auto bundleCode = master->bundleCode;
    //         auto components = BundleRegistry::instance().componentsOf(bundleCode);

    //         // 🔗 minden plan-ben lévő bundle darabhoz külön bundleInstanceId
    //         for (const auto& piece : plan.piecesWithMaterial)
    //         {
    //             QUuid bundleInstanceId = QUuid::createUuid();
    //             int pieceLen = piece.info.length_mm;

    //             for (const auto& comp : components)
    //             {
    //                 QUuid compMatId = comp.materialId;
    //                 const MaterialMaster* compMaster =
    //                     MaterialRegistry::instance().findById(compMatId);

    //                 if (!compMaster)
    //                     continue;

    //                 // ✔ helyes mező: stockLength_mm
    //                 int compStrandLen = static_cast<int>(compMaster->stockLength_mm);

    //                 // [PATCH] leftover-bundle túlvágás detektálása (CutPlan szintű lineage alapján)
    //                 std::optional<LeftoverStockEntry> loOpt;

    //                 if (plan.source == Cutting::Plan::Source::Reusable && !plan.sourceBarcode.isEmpty())
    //                 {
    //                     loOpt = LeftoverStockRegistry::instance().findByBarcode(plan.sourceBarcode);
    //                 }

    //                 if (loOpt.has_value())
    //                 {
    //                     const auto& lo = loOpt.value();

    //                     // csak akkor releváns, ha ugyanaz a bundle anyag
    //                     if (lo.materialId == master->id)
    //                     {
    //                         int loCompLen = lo.getComponentLength(compMatId);

    //                         // ha a leftover komponens rövidebb → az a szűk keresztmetszet
    //                         compStrandLen = std::min(compStrandLen, loCompLen);
    //                     }
    //                 }

    //                 // ✔ túlvágás detektálása
    //                 if (pieceLen > compStrandLen)
    //                 {
    //                     result.hasOvercuts = true;

    //                     // 1️⃣ eredeti request megkeresése
    //                     const Cutting::Plan::Request* originalReq = nullptr;

    //                     for (const auto& r : model.getRequests())   // <-- ez a helyes lista
    //                     {
    //                         if (r.requestId == piece.info.requestId)
    //                         {
    //                             originalReq = &r;
    //                             break;
    //                         }
    //                     }

    //                     Cutting::Plan::Request req;

    //                     if (originalReq)
    //                     {
    //                         // 2️⃣ teljes klónozás
    //                         req = *originalReq;

    //                         //req.requestId = QUuid::createUuid();
    //                         req.materialId = compMatId;

    //                     }
    //                     else
    //                     {
    //                         // fallback skeleton
    //                         req.requestId = QUuid::createUuid();
    //                         req.materialId = compMatId;
    //                     }

    //                     // if (originalReq)
    //                     // {
    //                     //     // 2️⃣ teljes klónozás
    //                     //     req = *originalReq;

    //                     //     // 🔧 MINDIG ÚJ requestId a pótló requesthez
    //                     //     req.requestId = QUuid::createUuid();
    //                     //     req.materialId = compMatId;
    //                     // }
    //                     // else
    //                     // {
    //                     //     // fallback skeleton
    //                     //     req.requestId = QUuid::createUuid();
    //                     //     req.materialId = compMatId;
    //                     // }

    //                     // 3️⃣ bundle‑komponens specifikus mezők átírása
    //                     req.requiredLength = pieceLen;
    //                     req.quantity = comp.count;

    //                     // 4️⃣ extRef öröklése + bundle‑jelölés
    //                     req.externalReference = piece.info.externalReference;
    //                         // originalReq && !originalReq->externalReference.isEmpty()
    //                         //     ? originalReq->externalReference
    //                         //     : QString("AUTO-BUNDLE-OVER-%1").arg(bundleCode);

    //                     // 5️⃣ hozzáadás
    //                     result.newRequests.append(req);

    //                     // 6️⃣ bundleInstanceId összekötés
    //                     result.bundleMap[bundleInstanceId].append(req.requestId);

    //                     zWarning(QStringLiteral(
    //                                  "⚠️ BundleOverCuttingDetector: túlvágás detektálva "
    //                                  "bundle=%1 komponens=%2 pieceLen=%3 strandLen=%4")
    //                                  .arg(master->toDisplay())
    //                                  .arg(compMaster->toDisplay())
    //                                  .arg(pieceLen)
    //                                  .arg(compStrandLen));
    //                 }

    //             }
    //         }
    //     }

    //     // 📋 RÉSZLETES ÖSSZEFOGLALÓ LOG A VÉGÉN
    //     if (!result.hasOvercuts) {
    //         zInfo("BundleOverCuttingDetector::detect — nincs detektált bundle túlvágás");
    //     } else {
    //         zInfo(QString("BundleOverCuttingDetector::detect — túlvágás összegzés: bundles=%1, newRequests=%2")
    //                   .arg(result.bundleMap.size())
    //                   .arg(result.newRequests.size()));

    //         // 🔍 Részletes request lista
    //         for (const auto& req : result.newRequests) {

    //             const MaterialMaster* mm = MaterialRegistry::instance().findById(req.materialId);
    //             QString matName = mm ? mm->toDisplay() : "?";

    //             zInfo(QString("  → Pótló request: material=%1, requiredLength=%2, quantity=%3, extRef=%4, requestId=%5")
    //                       .arg(matName)
    //                       .arg(req.requiredLength)
    //                       .arg(req.quantity)
    //                       .arg(req.externalReference)
    //                       .arg(req.requestId.toString()));
    //         }

    //         // 🔍 BundleInstance → RequestId lista
    //         for (auto it = result.bundleMap.constBegin(); it != result.bundleMap.constEnd(); ++it) {

    //             const QUuid& bundleInstanceId = it.key();
    //             const QList<QUuid>& reqIds = it.value();

    //             zInfo(QString("  • bundleInstanceId=%1 → %2 db pótló request")
    //                       .arg(bundleInstanceId.toString())
    //                       .arg(reqIds.size()));

    //             for (const auto& rid : reqIds) {
    //                 zInfo(QString("      ↳ requestId=%1").arg(rid.toString()));
    //             }
    //         }
    //     }


    //     return result;
    // }

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



};
