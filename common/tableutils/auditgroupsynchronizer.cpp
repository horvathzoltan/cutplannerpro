#include "common/tableutils/auditgroupsynchronizer.h"
#include "common/tableutils/auditcellformatter.h"
#include "common/tableutils/tableutils_auditcells.h"

#include <view/managers/storageaudittable_manager.h>

namespace TableUtils {

AuditGroupSynchronizer::AuditGroupSynchronizer(QTableWidget* table,
                                               const QMap<QUuid, StorageAuditRow>& rowMap,
                                               const QMap<QUuid, int>& rowIndexMap,
                                               AuditGroupLabeler* labeler)
    : _table(table), _rowMap(rowMap), _rowIndexMap(rowIndexMap),_labeler(labeler) {}

void AuditGroupSynchronizer::syncGroup(const AuditContext& ctx) {
    for (const QUuid& rowId : ctx.group.rowIds) {
        if (_rowMap.contains(rowId))
            syncRow(_rowMap.value(rowId));
    }
}

void AuditGroupSynchronizer::syncRow(const StorageAuditRow& row) {
    if (!_table || !_rowIndexMap.contains(row.rowId))
        return;

    int rowIx = _rowIndexMap.value(row.rowId);

    QString groupLabel = row.context ? _labeler->labelFor(row.context.get()) : "";
    QString expectedText = AuditCellFormatter::formatExpectedQuantity(row, groupLabel);

//    QString expectedText = AuditCellFormatter::formatExpectedQuantity(row);
    QString missingText  = AuditCellFormatter::formatMissingQuantity(row);
    QString statusText   = TableUtils::AuditCells::statusText(row);

    if (auto* itemExpected = _table->item(rowIx, StorageAuditTableManager::ColExpected))
        itemExpected->setText(expectedText);

    if (auto* itemMissing = _table->item(rowIx, StorageAuditTableManager::ColMissing))
        itemMissing->setText(missingText);

    if (auto* itemStatus = _table->item(rowIx, StorageAuditTableManager::ColStatus))
        itemStatus->setText(statusText);
}

} // namespace TableUtils
