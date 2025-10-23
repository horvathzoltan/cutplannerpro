#include "storageaudittable_manager.h"

#include "common/logger.h"
#include "common/tableutils/storageaudittable_rowstyler.h"
//#include "common/tableutils/tableutils.h"
//#include "common/tableutils/tableutils_auditcells.h"
#include "common/tableutils/auditcellformatter.h"

#include "model/storageaudit/storageauditentry.h"
#include "model/storageaudit/storageauditrow.h"
#include "model/registries/materialregistry.h"

#include <QCheckBox>
#include <QRadioButton>
#include <QSpinBox>
#include "view/tablehelpers/tablerowpopulator.h"
#include "view/viewmodels/audit/rowgenerator.h"

bool StorageAuditTableManager::_isVerbose = false;

StorageAuditTableManager::StorageAuditTableManager(QTableWidget* table, QWidget* parent)
    : QObject(parent), _table(table), _parent(parent)
{
    // 🔗 Csoport szinkronizáló inicializálása – a sorok közötti logikai kapcsolatért felel
    _groupSync = std::make_unique<TableUtils::AuditGroupSynchronizer>(
        _table,
        _auditRowMap,
        _rows.rowIndexMap(),
        &_groupLabeler,
        this // szükséges, hogy a signalokat vissza tudjuk vezetni
    );
}

void StorageAuditTableManager::addRow(const StorageAuditRow& row) {
    if (!_table)
        return;

    // 🔍 Anyag lekérése az azonosító alapján
    const MaterialMaster* mat = MaterialRegistry::instance().findById(row.materialId);
    if (!mat)
        return;

    // ➕ Új sor beszúrása
    int rowIx = _table->rowCount();
    _table->insertRow(rowIx);

    // 🗂️ AuditRow mentése a belső map-be
    _auditRowMap[row.rowId] = row;

    // 🏷️ Csoportcímke lekérése
    QString groupLabel = row.context ? _groupLabeler.labelFor(row.context.get()) : "";

    // 🧱 ViewModel generálása + 🧩 Megjelenítés
    TableRowViewModel vm = Audit::ViewModel::RowGenerator::generate(row, mat, groupLabel, this);
    TableRowPopulator::populateRow(_table, rowIx, vm);

    // 🧭 Sorregisztráció – segít a sorok azonosításában
    _rows.registerRow(rowIx, row.rowId);

    // 🔁 Csoport szinkronizálása – ha van AuditContext
    if (row.context)
        _groupSync->syncGroup(*row.context, row.rowId);

    // 🧠 Naplózás – csak ha van AuditContext
    if (_isVerbose && row.context) {
        zInfo(L("AuditContext [%1]: expected=%2, actual=%3, rows=%4")
            .arg(row.materialId.toString())
            .arg(row.context->totalExpected)
            .arg(row.context->totalActual)
            .arg(row.context->group.size()));
    }
}

void StorageAuditTableManager::updateRow(const StorageAuditRow& row) {
    if (!_table)
        return;

    const MaterialMaster* mat = MaterialRegistry::instance().findById(row.materialId);
    if (!mat)
        return;

    // 🧠 Naplózás – frissített AuditContext
    if (_isVerbose && row.context) {
        zInfo(L("🔄 Frissített AuditContext [%1]: expected=%2, actual=%3, rows=%4")
            .arg(row.materialId.toString())
            .arg(row.context->totalExpected)
            .arg(row.context->totalActual)
            .arg(row.context->group.size()));
    }

    // 🔍 Sorindex lekérése a rowId alapján
    std::optional<int> rowIxOpt = _rows.rowOf(row.rowId);
    if (!rowIxOpt.has_value())
        return;

    int rowIx = rowIxOpt.value();

    // 🗂️ Frissített AuditRow mentése
    _auditRowMap[row.rowId] = row;

    // 🏷️ Csoportcímke újra lekérése
    QString groupLabel = row.context ? _groupLabeler.labelFor(row.context.get()) : "";

    // 🧱 ViewModel generálása + 🧩 Megjelenítés
    TableRowViewModel vm = Audit::ViewModel::RowGenerator::generate(row, mat, groupLabel, this);
    TableRowPopulator::populateRow(_table, rowIx, vm);

    // 🔁 Csoport újraszinkronizálása
    if (row.context)
        _groupSync->syncGroup(*row.context, row.rowId);

}

void StorageAuditTableManager::clearTable() {
    _table->clearContents();
    _table->setRowCount(0);
    _rows.clear();
    _groupLabeler.clear();
}



void StorageAuditTableManager::showAuditCheckbox(const QUuid& rowId)
{
    std::optional<int> rowIxOpt = _rows.rowOf(rowId);
    if (!rowIxOpt.has_value())
        return;

    int rowIx = rowIxOpt.value();

    //int rowIndex = findRowIndexById(rowId);
    //if (rowIndex < 0) return;

    auto* checkbox = new QCheckBox("Auditálva");
    checkbox->setProperty("rowId", rowId);

    QObject::connect(checkbox, &QCheckBox::toggled, this, [this](bool checked) {
        auto* cb = qobject_cast<QCheckBox*>(sender());
        if (!cb) return;
        QUuid rowId = cb->property("rowId").toUuid();
        emit auditCheckboxToggled(rowId, checked);
    });

    _table->setCellWidget(rowIx, AuditTableColumns::Actual, checkbox);
}

