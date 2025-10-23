#pragma once

#include "service/storageaudit/auditcontextbuilder.h"
#include "common/logger.h"
#include "model/cutting/plan/cutplan.h"
#include "model/storageaudit/storageauditrow.h"

#include <QMap>

namespace AuditUtils {

static const bool _isVerbose = false;

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
                    //row.rowPresence         = AuditPresence::Present;
                    break;
                }
            }
            break;
        }
        case AuditSourceType::Stock: {
            if (requiredStockMaterials.contains(row.materialId)) {
                // minden sor a csoportban része az optimalizációnak
                row.isInOptimization = true;
                row.pickingQuantity = 0; // sor szinten nincs kiosztás
                // a totalExpected-et majd a context kapja meg
            }
            break;
        }

        // case AuditSourceType::Stock: {
        //     if (requiredStockMaterials.contains(row.materialId)) {
        //         int& remaining = requiredStockMaterials[row.materialId];
        //         if (remaining > 0) {
        //             row.pickingQuantity = 1;
        //             row.isInOptimization = true;
        //             row.presence = (row.actualQuantity >= 1)
        //                                ? AuditPresence::Present
        //                                : AuditPresence::Missing;
        //             --remaining;
        //         }
        //     }
        //     break;
        // }
        // case AuditSourceType::Stock: {
        //     if (requiredStockMaterials.contains(row.materialId)) {
        //         int& remaining = requiredStockMaterials[row.materialId];
        //         if (remaining > 0) {
        //             row.isInOptimization = true;
        //             row.pickingQuantity = 0;//remaining; // teljes igény
        //             // row.rowPresence = (row.actualQuantity >= remaining)
        //             //                    ? AuditPresence::Present
        //             //                    : AuditPresence::Missing;
        //             remaining = 0; // egyszer kiosztva
        //         }
        //     }
        //     break;
        // }


        default:
            // Egyéb forrástípusok (ha lesznek a jövőben)
            row.isInOptimization = false;
            break;
        }

        if(_isVerbose){
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
                           .arg(row.context->totalExpected)
                           .arg(row.context->totalActual)
                           .arg(row.context->group.size()));
            }

            // ⚠️ Warning: hulló sor csoportba került (nem kéne)
            if (row.sourceType == AuditSourceType::Leftover &&
                row.context && row.context->group.isGroup()) {
                zWarning(QString("⚠️ Hulló sor csoportba került! rowId=%1 | matId=%2 | groupSize=%3")
                             .arg(row.rowId.toString())
                             .arg(row.materialId.toString())
                             .arg(row.context->group.size()));
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
inline void assignContextsToRows(QVector<StorageAuditRow>* auditRows,
                                 const QMap<QUuid,int>& requiredStockMaterials)
{
    if (!auditRows) {
        zWarning(L("⚠️ Kontextus hozzárendelése sikertelen: auditRows=nullptr"));
        return;
    }

    auto contextMap = AuditContextBuilder::buildFromRows(*auditRows, requiredStockMaterials);
    for (auto& row : *auditRows) {
        row.context = contextMap.value(row.rowId);
    }

    // 🔄 Csoportosítás kulcs alapján: minden context megkapja a hozzá tartozó sorokat
    QMap<QString, QList<StorageAuditRow*>> groupMap;
    for (auto& row : *auditRows) {
        QString key = AuditContextBuilder::makeGroupKey(row);
        groupMap[key].append(&row);
    }

    for (auto& row : *auditRows) {
        QString key = AuditContextBuilder::makeGroupKey(row);
        if (row.context) {
            row.context->setGroupRows(groupMap.value(key));
        }
    }

    zInfo(L("🔗 AuditContext hozzárendelve minden sorhoz — összes sor: %1")
              .arg(auditRows->size()));

    // for (auto& row : *auditRows) {
    //     zInfo(QString("rowId=%1 | matId=%2 | rootStorageId=%3 | groupKey=%4 | expected=%5 | actual=%6 | contextPtr=%7")
    //               .arg(row.rowId.toString())
    //               .arg(row.materialId.toString())
    //               .arg(row.rootStorageId.toString())
    //               .arg(AuditContextBuilder::makeGroupKey(row))
    //               .arg(row.context ? row.context->totalExpected : -1)
    //               .arg(row.context ? row.context->totalActual : -1)
    //               .arg((quintptr)(row.context ? row.context.get() : nullptr), 0, 16));
    // }

}


} // namespace AuditUtils
