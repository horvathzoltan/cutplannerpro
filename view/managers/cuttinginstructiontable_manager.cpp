#include "cuttinginstructiontable_manager.h"

#include "view/viewmodels/cutting/rowgenerator.h"
#include "view/tablehelpers/tablerowpopulator.h"
#include "common/logger.h"
#include "model/registries/materialregistry.h"
#include "view/tableutils/colorlogicutils.h"

#include <view/tableutils/highlightdelegate.h>


bool CuttingInstructionTableManager::_isVerbose = false;

/**
 * @brief Konstruktor – inicializálja a táblát.
 */
CuttingInstructionTableManager::CuttingInstructionTableManager(QTableWidget* table,
                                                               QWidget* parent)
    : QObject(parent), _table(table), _parent(parent) {
    // A tábla oszlopait a designerben definiáljuk (StepId, RodLabel, Material, stb.)
}

void CuttingInstructionTableManager::addMachineRow(const MachineHeader& machine) {
    if (!_table) return;

    int rowIx = _table->rowCount();
    _table->insertRow(rowIx);
    _table->setSpan(rowIx, 0, 1, _table->columnCount());

    TableRowViewModel vm = Cutting::ViewModel::RowGenerator::generateMachineSeparator(machine, this);
    TableRowPopulator::populateRow(_table, rowIx, vm);

    if (QTableWidgetItem* item = _table->item(rowIx, 0)) {
        item->setData(Qt::UserRole + 2, true);
    }


    if (_isVerbose) {
        zInfo(QString("CuttingInstruction machine separator row added: %1")
                  .arg(machine.machineName));
    }
}
/**
 * @brief Új sor beszúrása a CuttingInstruction táblába.
 *
 * - Meghívja a RowGenerator::generate()-t
 * - A kapott TableRowViewModel celláit beírja a QTableWidget-be
 * - A rowId-t eltárolja a belső map-ekben
 */
void CuttingInstructionTableManager::addRow(const CutInstruction& ci) {
    if (!_table) return;

    int rowIx = _table->rowCount();
    _table->insertRow(rowIx);

    CutInstruction ciCopy = ci;
    if (_currentRowIx < 0) {
        ciCopy.status = CutStatus::InProgress;
        _currentRowIx = rowIx;
        emit rowFinalized(rowIx);
    }

    // ViewModel generálása és cellák kirenderelése
    TableRowViewModel vm = Cutting::ViewModel::RowGenerator::generate(ciCopy, this);
    TableRowPopulator::populateRow(_table, rowIx, vm);

    // rowId mentése
    _rowMap.insert(vm.rowId, ciCopy);
    _rowIndexMap[vm.rowId] = rowIx;

    if (_isVerbose) {
        const auto* mat = MaterialRegistry::instance().findById(ci.materialId);
        zInfo(QString("CuttingInstruction row added: %1 | step=%2")
                  .arg(mat->name)
                  .arg(ciCopy.globalStepId));
    }
}

/**
 * @brief Meglévő sor frissítése rowId alapján.
 *
 * - Újra legenerálja a ViewModel-t a frissített CutInstruction-ből
 * - Frissíti a cellák tartalmát a táblában
 */
void CuttingInstructionTableManager::updateRow(const QUuid& rowId, const CutInstruction& ci) {
    if (!_table) return;
    if (!_rowIndexMap.contains(rowId)) return;

    int rowIx = _rowIndexMap.value(rowId);
    _rowMap.insert(rowId, ci);

    TableRowViewModel vm = Cutting::ViewModel::RowGenerator::generate(ci, this);
    TableRowPopulator::populateRow(_table, rowIx, vm);

    if (_isVerbose) {
        const auto* mat = MaterialRegistry::instance().findById(ci.materialId);
        zInfo(QString("CuttingInstruction row updated: %1 | step=%2")
                  .arg(mat->name)
                  .arg(ci.globalStepId));
    }
}

/**
 * @brief Teljes tábla törlése.
 */
void CuttingInstructionTableManager::clearTable() {
    if (!_table) return;

    _table->clearContents();
    _table->setRowCount(0);
    _rowMap.clear();
    _rowIndexMap.clear();

    if (_isVerbose) {
        zInfo("CuttingInstruction table cleared");
    }
}

/**
 * @brief finalizeRow – egy sor végrehajtása (vágás + leftover regisztráció).
 *
 * - Ellenőrzi az előfeltételeket (barcode, machineName, hossz konzisztencia)
 * - Lefuttatja a computeRemaining()-et
 * - Ha minden rendben, státuszt Done-ra állítja és frissíti a sort
 */
void CuttingInstructionTableManager::finalizeRow(const QUuid& rowId) {
    auto it = _rowMap.find(rowId);
    if (it == _rowMap.end()) {
        zWarning(QString("finalizeRow: row not found %1").arg(rowId.toString()));
        return;
    }

    CutInstruction& ci = it.value();

    const auto* mat = MaterialRegistry::instance().findById(ci.materialId);


    // Előfeltételek
    if (ci.barcode.isNull() || mat->name.isEmpty()) {
        zWarning("finalizeRow: missing barcode or machine");
        return;
    }

    ci.computeRemaining();
    if (ci.lengthAfter_mm < 0) {
        zWarning("finalizeRow: negative remaining; check cutSize/kerf");
        return;
    }

    // TODO: CuttingExecutionService meghívása (perzisztálás, leftover regisztráció)
    // Most csak státuszt állítunk
    ci.status = CutStatus::Done;
    updateRow(rowId, ci);

    zInfo(QString("finalizeRow: success for row %1 | step=%2 | remainingAfter=%3")
              .arg(rowId.toString())
              .arg(ci.globalStepId)
              .arg(ci.lengthAfter_mm));

    // léptetés: következő sor a táblázatban
    int currentRowIx = _rowIndexMap.value(rowId, -1);
    int nextRowIx = -1;

    for (int r = currentRowIx + 1; r < _table->rowCount(); ++r) {
        QTableWidgetItem* item = _table->item(r, 0);
        if (!item) continue;
        if (!item->data(Qt::UserRole + 2).toBool()) { // nem gép-header
            nextRowIx = r;
            break;
        }
    }

    // finalizeRow végén
    if (nextRowIx != -1) {
        _currentRowIx = nextRowIx;
        QUuid nextRowId;
        for (auto it = _rowIndexMap.begin(); it != _rowIndexMap.end(); ++it) {
            if (it.value() == nextRowIx) {
                nextRowId = it.key();
                break;
            }
        }
        if (!nextRowId.isNull()) {
            CutInstruction& c2 = _rowMap[nextRowId];
            c2.status = CutStatus::InProgress;
            updateRow(nextRowId, c2); // csak a következő sor frissül
        }
    } else {
        _currentRowIx = -1; // nincs több sor
    }

    emit rowFinalized(currentRowIx);
}

void CuttingInstructionTableManager::compensationChange(const QUuid& machineId, double newVal){
    emit compensationChanged(machineId, newVal);
}
