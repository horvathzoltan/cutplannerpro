#pragma once

#include "view/tableutils/auditgrouplabeler.h"
#include "model/storageaudit/storageauditrow.h"
#include <QTableWidget>
#include <QMap>
#include <QUuid>

class StorageAuditTableManager; // előredeklarálás


namespace TableUtils {

/**
 * @brief AuditContext csoporthoz tartozó sorok szinkronizált megjelenítését végzi.
 *
 * Feladatok:
 * - Azonos AuditContext csoportba tartozó sorok celláit egységesen frissíti.
 * - Elvárt, hiányzó és státusz cellák összehangolt értéket kapnak.
 * - A jövőben bővíthető tooltip, szín, aggregált actual kezelésére.
 */

class AuditGroupSynchronizer {
public:
    AuditGroupSynchronizer(QTableWidget* table,
                           const QMap<QUuid, StorageAuditRow>& rowMap,
                           const QMap<QUuid, int>& rowIndexMap,
                           AuditGroupLabeler* labeler,
                           StorageAuditTableManager* manager);

    void syncGroup(const AuditContext& ctx, const QUuid& excludeRowId = {});
    void syncRow(const StorageAuditRow& row);

private:
    QTableWidget* _table = nullptr;
    const QMap<QUuid, StorageAuditRow>& _rowMap;
    const QMap<QUuid, int>& _rowIndexMap;
    AuditGroupLabeler* _labeler = nullptr;
    StorageAuditTableManager* _manager = nullptr;
};


} // namespace TableUtils
