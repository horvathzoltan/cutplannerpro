#include "auditgroupsynchronizer.h"
#include "../viewmodels/audit/rowgenerator.h"
#include "../managers/storageaudittable_manager.h" // 🔹 ez kell a metódushíváshoz
#include "../tablehelpers/tablerowpopulator.h"

#include "materials/registry/material_registry.h"

namespace TableUtils {

// 🔧 Konstruktor: minden szükséges adatot megkap, beleértve a manager példányt is
AuditGroupSynchronizer::AuditGroupSynchronizer(QTableWidget* table,
                                               const QMap<QUuid, StorageAuditRow>& rowMap,
                                               const QMap<QUuid, int>& rowIndexMap,
                                               AuditGroupLabeler* labeler,
                                               StorageAuditTableManager* manager)
    : _table(table),
    _rowMap(rowMap),
    _rowIndexMap(rowIndexMap),
    _labeler(labeler),
    _manager(manager) {}


// 🔁 Csoport szinkronizálása – kivéve az aktuális sort, hogy ne legyen duplikált frissítés
void AuditGroupSynchronizer::syncGroup(const AuditContext& ctx, const QUuid& excludeRowId) {
    for (const QUuid& rowId : ctx.group.rowIds()) {
        if (rowId == excludeRowId)
            continue;

        if (_rowMap.contains(rowId))
            syncRow(_rowMap.value(rowId));
    }
}


// 🔄 Egy sor szinkronizálása – csak tartalomfrissítés, struktúra nem változik
void AuditGroupSynchronizer::syncRow(const StorageAuditRow& row) {
    if (!_table || !_rowIndexMap.contains(row.rowId) || !_manager)
        return;

    int rowIx = _rowIndexMap.value(row.rowId);

    // 🔹 Context‑független, null‑safe API a row‑on keresztül
    QString groupLabel;
    if (row.hasContext()) {
        groupLabel = _labeler->labelFor(row.contextPtr());
    }

    const MaterialMaster* mat = MaterialRegistry::instance().findById(row.materialId);
    if (!mat)
        return;

    // 🔄 ViewModel újragenerálása
    TableRowViewModel vm = Audit::ViewModel::RowGenerator::generate(row, *mat, groupLabel, _manager);

    // 🧩 Cellák újratöltése
    TableRowPopulator::populateRow(_table, rowIx, vm);
}


} // namespace TableUtils
