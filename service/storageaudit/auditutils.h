#pragma once

#include "service/storageaudit/auditcontextbuilder.h"
#include "common/logger.h"
#include "model/cutting/plan/cutplan.h"
#include "model/storageaudit/storageauditrow.h"

#include <QMap>

namespace AuditUtils {

static const bool _isVerbose = true;

/**
 * @brief A vágási tervek injektálása az audit sorokba.
 *
 * Feladata:
 * - Stock sorok esetén: beállítja az isInOptimization flag-et, ha az adott materialId szerepel a tervekben.
 *   Az elvárt darabszámot (expected) nem itt, hanem a context építésnél számoljuk ki a pickingMap alapján.
 * - Leftover sorok esetén: példány szinten vizsgálja, hogy a plan ténylegesen használja-e az adott rodId/barcode-ot.
 *   Ha igen → isInOptimization=true, actualQuantity=1, expected=1 (bináris jelenlét).
 *   Ha nem → isInOptimization=false, actualQuantity=0, expected=0.
 *
 * Ez biztosítja, hogy a leftover sorok ne örököljenek stock igényeket,
 * hanem mindig önálló, 0/1 elvárással szerepeljenek.
 *
 * @param plans   Az optimalizációs tervek listája.
 * @param auditRows Az audit sorok vektora, amelyet frissítünk.
 */

inline void injectPlansIntoAuditRows(const QVector<Cutting::Plan::CutPlan>& plans,
                                     QVector<StorageAuditRow>* auditRows)
{
    if (!auditRows) {
        zWarning(L("⚠️ Audit sorok injektálása sikertelen: auditRows=nullptr"));
        return;
    }
    if (_isVerbose) {
        zInfo(L("=== AuditRows BEFORE injectPlansIntoAuditRows ==="));
        for (auto& row : *auditRows) {
            zInfo(QString("rowId=%1 | srcType=%2 | matId=%3 | groupKey=%4 | "
                          "expected=%5 | actual=%6 | inOpt=%7 | ctx=%8 | groupSize=%9")
                      .arg(row.rowId.toString())
                      .arg((int)row.sourceType)
                      .arg(row.materialId.toString())
                      .arg(AuditContextBuilder::makeGroupKey(row))
                      .arg(row.totalExpected())
                      .arg(row.totalActual())
                      .arg(row.isInOptimization)
                      .arg((quintptr)(row.contextPtr()), 0, 16)
                      .arg(row.hasContext() ? row.groupSize() : -1));
        }
    }

    // 🔹 Összesített anyagigény stock esetén (materialId → darabszám)
    QMap<QUuid, int> requiredStockMaterials;
    for (const auto& plan : plans) {
        if (plan.source == Cutting::Plan::Source::Stock) {
            requiredStockMaterials[plan.materialId] += 1; // minden CutPlan = 1 rúd
        }
    }

    // 🔄 Audit sorok frissítése a vágási terv alapján
    for (auto& row : *auditRows) {
        switch (row.sourceType) {
        case AuditSourceType::Leftover: {
            bool used = false;
            for (const auto& plan : plans) {
                if (plan.source == Cutting::Plan::Source::Reusable &&
                    plan.sourceBarcode == row.barcode) {
                    row.isInOptimization = true;
                    row.actualQuantity = 1;   // leftover mindig 1 db
                    // csak jelöld, hogy van elvárás
                    // pl. row.expectedQuantity = 1;  (ha van ilyen meződ)
                    used = true;
                    break;
                }
            }
            if (!used) {
                row.isInOptimization = false;
                row.actualQuantity = 0;
                // row.expectedQuantity = 0;
            }
            break;
        }

        case AuditSourceType::Stock: {
            // Stock sorok: materialId alapján jelöljük
            if (requiredStockMaterials.contains(row.materialId)) {
                row.isInOptimization = true;
                //row.pickingQuantity = 0; // sor szinten nincs kiosztás
                // a totalExpected-et majd a context kapja meg
            } else {
                row.isInOptimization = false;
            }
            break;
        }
        default:
            row.isInOptimization = false;
            break;
        }

        if (_isVerbose) {
            zInfo(QString("[AuditInject] rowId=%1 | barcode=%2 | srcType=%3 | matId=%4 | "
                          "expected(picking)=%5 | actual=%6 | inOpt=%7")
                      .arg(row.rowId.toString())
                      .arg(row.barcode) // 🔹 így azonnal látod, hogy RST-013-ról van szó
                      .arg((int)row.sourceType)
                      .arg(row.materialId.toString())
                      .arg(row.totalExpected())
                      .arg(row.actualQuantity)
                      .arg(row.isInOptimization));

            // 🧠 Debug: AuditContext aggregált értékek, ha elérhető
            if (row.hasContext()) {
                zInfo(QString("[AuditContext] matId=%1 | expected=%2 | actual=%3 | rows=%4")
                          .arg(row.materialId.toString())
                          .arg(row.totalExpected())
                          .arg(row.totalActual())
                          .arg(row.groupSize()));
            }

            // ⚠️ Warning: hulló sor csoportba került (nem kéne)
            if (row.sourceType == AuditSourceType::Leftover && row.isGrouped()) {
                zWarning(QString("⚠️ Hulló sor csoportba került! rowId=%1 | matId=%2 | groupSize=%3")
                             .arg(row.rowId.toString())
                             .arg(row.materialId.toString())
                             .arg(row.groupSize()));
            }

            // ⚠️ Warning: negatív hiány (hibás aggregálás vagy túl sok actual)
            int missing = row.missingQuantity();
            if (missing < 0) {
                zWarning(QString("⚠️ Negatív hiány! rowId=%1 | matId=%2 | missing=%3")
                             .arg(row.rowId.toString())
                             .arg(row.materialId.toString())
                             .arg(missing));
            }
        }
    } // end for auditRows

    if (_isVerbose) {
        zInfo(L("=== AuditRows AFTER injectPlansIntoAuditRows ==="));
        for (auto& row : *auditRows) {
            zInfo(QString("rowId=%1 | srcType=%2 | matId=%3 | groupKey=%4 | "
                          "expected=%5 | actual=%6 | inOpt=%7 | ctx=%8 | groupSize=%9")
                      .arg(row.rowId.toString())
                      .arg((int)row.sourceType)
                      .arg(row.materialId.toString())
                      .arg(AuditContextBuilder::makeGroupKey(row))
                      .arg(row.totalExpected())
                      .arg(row.totalActual())
                      .arg(row.isInOptimization)
                      .arg((quintptr)(row.contextPtr()), 0, 16)
                      .arg(row.hasContext() ? row.groupSize() : -1));
        }
    }

    zInfo(L("🔄 Audit sorok frissítve a vágási terv alapján — összes sor: %1")
              .arg(auditRows->size()));
}

/**
 * @brief Kontextus hozzárendelése audit sorokhoz.
 *
 * Feladata:
 * - Stock sorok: materialId + rootStorageId alapján csoportosítja őket.
 *   A context totalExpected értékét a pickingMap (requiredStockMaterials) alapján számolja.
 * - Leftover sorok: mindig saját rowId alapján külön contextbe kerülnek,
 *   így nem aggregálódnak materialId szerint és nem keverednek a stock sorokkal.
 *   Az expected értékük bináris (0/1), amit az injectPlansIntoAuditRows állít be.
 *
 * A context tartalmazza:
 * - AuditGroupInfo: materialId, groupKey, rowIds
 * - totalExpected: összesített elvárt mennyiség (stocknál darabszám, leftovernél 0/1)
 * - totalActual: tényleges mennyiség (stocknál készlet, leftovernél 1)
 * - a csoporthoz tartozó sorok listáját
 *
 * Ez az alapja a státusz, hiányzó mennyiség, tooltip és UI megjelenítésnek.
 *
 * @param auditRows Az audit sorok vektora, amelyhez contextet rendelünk.
 * @param requiredStockMaterials A pickingMap, amely a stock anyagok elvárt darabszámát tartalmazza.
 */

inline void assignContextsToRows(QVector<StorageAuditRow>* auditRows,
                                 const QMap<QUuid,int>& requiredStockMaterials)
{
    if (!auditRows) {
        zWarning(L("⚠️ Kontextus hozzárendelése sikertelen: auditRows=nullptr"));
        return;
    }

    if(_isVerbose){
        zInfo(L("=== AuditRows BEFORE inject ==="));
        for (auto& row : *auditRows) {
            zInfo(QString("rowId=%1 | matId=%2 | srcType=%3 | expected=%4 | actual=%5 | hasContext=%6 | groupKey=%7")
                      .arg(row.rowId.toString())
                      .arg(row.materialId.toString())
                      .arg((int)row.sourceType)
                      .arg(row.totalExpected())
                      .arg(row.totalActual())
                      .arg(row.hasContext())
                      .arg(AuditContextBuilder::makeGroupKey(row)));
        }
    }

    auto contextMap = AuditContextBuilder::buildFromRows(*auditRows, requiredStockMaterials);

    if(_isVerbose){
        zInfo(L("=== ContextMap built ==="));
        for (auto it = contextMap.begin(); it != contextMap.end(); ++it) {
            const auto& ctx = it.value();
            zInfo(QString("ctx for rowId=%1 | expected=%2 | actual=%3 | groupSize=%4 | ptr=%5")
                      .arg(it.key().toString())
                      .arg(ctx ? ctx->totalExpected : -1)
                      .arg(ctx ? ctx->totalActual : -1)
                      .arg(ctx ? ctx->group.size() : -1)
                      .arg((quintptr)(ctx.get()), 0, 16));
        }
    }

    // 🔹 Kontextus hozzárendelése setterrel
    for (auto& row : *auditRows) {
        row.setContext(contextMap.value(row.rowId));
    }

    // 🔄 Csoportosítás kulcs alapján: minden context megkapja a hozzá tartozó sorokat
    QMap<QString, QList<StorageAuditRow*>> groupMap;
    for (auto& row : *auditRows) {
        QString key = AuditContextBuilder::makeGroupKey(row);
        groupMap[key].append(&row);
    }

    for (auto& row : *auditRows) {
        if (row.hasContext()) {
            QString key = AuditContextBuilder::makeGroupKey(row);
            row.contextPtr()->setGroupRows(groupMap.value(key));
        }
    }

    if (_isVerbose) {
        zInfo(L("=== AuditRows AFTER assignContexts (ellenőrző log) ==="));
        for (auto& row : *auditRows) {
            // Alap dump
            zInfo(QString("rowId=%1 | barcode=%2 | srcType=%3 | matId=%4 | groupKey=%5 | "
                          "expected=%6 | actual=%7 | ctx=%8 | groupSize=%9")
                      .arg(row.rowId.toString())
                      .arg(row.barcode)
                      .arg((int)row.sourceType)
                      .arg(row.materialId.toString())
                      .arg(AuditContextBuilder::makeGroupKey(row))
                      .arg(row.totalExpected())
                      .arg(row.totalActual())
                      .arg((quintptr)(row.contextPtr()), 0, 16)
                      .arg(row.hasContext() ? row.groupSize() : -1));

            // 🔎 Ellenőrzés: ha leftover sor groupSize > 1 → gyanús
            if (row.sourceType == AuditSourceType::Leftover && row.groupSize() > 1) {
                zWarning(QString("⚠️ Leftover sor több elemű contextbe került! "
                                 "barcode=%1 | groupSize=%2 | ctx=%3")
                             .arg(row.barcode)
                             .arg(row.groupSize())
                             .arg((quintptr)(row.contextPtr()), 0, 16));
            }

            // 🔎 Ellenőrzés: ha stock és leftover ugyanarra a ctx pointerre mutat
            if (row.hasContext() && row.groupSize() > 1) {
                bool hasMixed = false;
                for (auto* peer : row.contextPtr()->groupRows()) {
                    if (peer->sourceType != row.sourceType) {
                        hasMixed = true;
                        break;
                    }
                }
                if (hasMixed) {
                    zWarning(QString("⚠️ Vegyes context! stock és leftover egy contextben "
                                     "ctx=%1 | barcode=%2")
                                 .arg((quintptr)(row.contextPtr()), 0, 16)
                                 .arg(row.barcode));
                }
            }
        }
    }


    zInfo(L("🔗 AuditContext hozzárendelve minden sorhoz — összes sor: %1")
              .arg(auditRows->size()));
}



} // namespace AuditUtils
