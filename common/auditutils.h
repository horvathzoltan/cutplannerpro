#pragma once

#include "auditcontextbuilder.h"
#include "common/logger.h"
#include "model/cutting/plan/cutplan.h"
#include <model/storageaudit/storageauditrow.h>

#include <QMap>

namespace AuditUtils {

/**
 * @brief Injektálja a vágási tervből származó elvárt mennyiségeket (pickingQuantity)
 *        és optimalizációs státuszokat az audit sorokba.
 *
 * Logika:
 * - Stock forrás: minden CutPlan egy rúd → 1 db igény
 * - Leftover forrás: minden hulló egyedi → 1 db igény, rodId alapján
 */
// inline void injectPlansIntoAuditRows(const QVector<Cutting::Plan::CutPlan>& plans,
//                                      QVector<StorageAuditRow>* auditRows)
// {
//     if (!auditRows) {
//         zWarning(L("⚠️ Audit sorok injektálása sikertelen: auditRows=nullptr"));
//         return;
//     }

//     // 🔹 Összesített anyagigény stock esetén (materialId → darabszám)
//     QMap<QUuid, int> requiredStockMaterials;
//     for (const auto& plan : plans) {
//         if (plan.source == Cutting::Plan::Source::Stock) {
//             requiredStockMaterials[plan.materialId] += 1; // minden CutPlan = 1 rúd
//         }
//     }

//     // 🔄 Audit sorok frissítése a vágási terv alapján
//     for (auto& row : *auditRows) {
//         switch (row.sourceType) {
//         case AuditSourceType::Leftover: {
//             // Hulló audit: barcode alapján összevetés
//             for (const auto& plan : plans) {
//                 if (plan.source == Cutting::Plan::Source::Reusable &&
//                     plan.rodId == row.barcode) {
//                     row.isInOptimization = true;
//                     row.pickingQuantity  = 1; // hullók mindig 1 db
//                     row.presence         = AuditPresence::Present;
//                     break;
//                 }
//             }
//             break;
//         }
//         case AuditSourceType::Stock: {
//             if (requiredStockMaterials.contains(row.materialId)) {
//                 int& remaining = requiredStockMaterials[row.materialId];
//                 if (remaining > 0) {
//                     row.pickingQuantity = 1;
//                     row.isInOptimization = true;
//                     row.presence = (row.actualQuantity >= 1)
//                                        ? AuditPresence::Present
//                                        : AuditPresence::Missing;
//                     --remaining;
//                 }
//             }
//             break;
//         }

//         default:
//             // Egyéb forrástípusok (ha lesznek a jövőben)
//             row.isInOptimization = false;
//             break;
//         }

//         // 🔍 Debug log minden sorhoz
//         zInfo(QString("[AuditInject] rowId=%1 | matId=%2 | expected(picking)=%3 | actual=%4 | inOpt=%5")
//                   .arg(row.rowId.toString())
//                   .arg(row.materialId.toString())
//                   .arg(row.pickingQuantity)
//                   .arg(row.actualQuantity)
//                   .arg(row.isInOptimization));
//     }

//     zInfo(L("🔄 Audit sorok frissítve a vágási terv alapján — összes sor: %1")
//               .arg(auditRows->size()));
// }

inline void injectPlansIntoAuditRows(const QVector<Cutting::Plan::CutPlan>& plans,
                                     QVector<StorageAuditRow>* auditRows)
{
    if (!auditRows) {
        zWarning(L("⚠️ Audit sorok injektálása sikertelen: auditRows=nullptr"));
        return;
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
            // Hulló audit: barcode alapján összevetés
            for (const auto& plan : plans) {
                if (plan.source == Cutting::Plan::Source::Reusable &&
                    plan.rodId == row.barcode) {
                    row.isInOptimization = true;
                    row.pickingQuantity  = 1; // hullók mindig 1 db
                    row.presence         = AuditPresence::Present;
                    break;
                }
            }
            break;
        }
        case AuditSourceType::Stock: {
            if (requiredStockMaterials.contains(row.materialId)) {
                int& remaining = requiredStockMaterials[row.materialId];
                if (remaining > 0) {
                    row.pickingQuantity = 1;
                    row.isInOptimization = true;
                    row.presence = (row.actualQuantity >= 1)
                                       ? AuditPresence::Present
                                       : AuditPresence::Missing;
                    --remaining;
                }
            }
            break;
        }

        default:
            // Egyéb forrástípusok (ha lesznek a jövőben)
            row.isInOptimization = false;
            break;
        }

        // 📋 Debug: injektált értékek soronként
        zInfo(QString("[AuditInject] rowId=%1 | matId=%2 | expected(picking)=%3 | actual=%4 | inOpt=%5")
                  .arg(row.rowId.toString())
                  .arg(row.materialId.toString())
                  .arg(row.pickingQuantity)
                  .arg(row.actualQuantity)
                  .arg(row.isInOptimization));

        // 🧠 Debug: AuditContext aggregált értékek, ha elérhető
        if (row.context) {
            zInfo(QString("[AuditContext] matId=%1 | expected=%2 | actual=%3 | rows=%4")
                       .arg(row.materialId.toString())
                       .arg(row.context->group.totalExpected)
                       .arg(row.context->group.totalActual)
                       .arg(row.context->group.rowIds.size()));
        }

        // ⚠️ Warning: hulló sor csoportba került (nem kéne)
        if (row.sourceType == AuditSourceType::Leftover &&
            row.context && row.context->group.rowIds.size() > 1) {
            zWarning(QString("⚠️ Hulló sor csoportba került! rowId=%1 | matId=%2 | groupSize=%3")
                         .arg(row.rowId.toString())
                         .arg(row.materialId.toString())
                         .arg(row.context->group.rowIds.size()));
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

    zInfo(L("🔄 Audit sorok frissítve a vágási terv alapján — összes sor: %1")
              .arg(auditRows->size()));
}

/**
 * @brief Kontextus hozzárendelése audit sorokhoz.
 *        Minden sor kap egy AuditContext pointert, amely tartalmazza a csoportosított adatokat.
 *
 * A csoportosítás kulcsa: materialId + storageName
 * A context tartalmazza:
 * - összesített elvárt mennyiséget (totalExpected)
 * - összesített tényleges mennyiséget (totalActual)
 * - az adott csoporthoz tartozó sorok azonosítóit (rowIds)
 *
 * Ez az alapja a státusz, hiányzó, tooltip és UI megjelenítésnek.
 */
inline void assignContextsToRows(QVector<StorageAuditRow>* auditRows)
{
    if (!auditRows) {
        zWarning(L("⚠️ Kontextus hozzárendelése sikertelen: auditRows=nullptr"));
        return;
    }

    auto contextMap = AuditContextBuilder::buildFromRows(*auditRows);
    for (auto& row : *auditRows) {
        row.context = contextMap.value(row.rowId);
    }

    zInfo(L("🔗 AuditContext hozzárendelve minden sorhoz — összes sor: %1")
              .arg(auditRows->size()));
}


} // namespace AuditUtils
