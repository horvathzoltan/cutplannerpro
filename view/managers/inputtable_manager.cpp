#include "inputtable_manager.h"
#include "../../common/logger.h"
//#include "view/columnindexes/inputtable_columns.h"
#include "../tablehelpers/tablerowpopulator.h"
//#include "view/tableutils/inputtable_rowstyler.h"
#include "../tableutils/tableutils.h"
//#include "model/registries/materialregistry.h"
#include "../viewmodels/tablerowviewmodel.h"
//#include "common/tableutils/resulttable_rowstyler.h"
#include <QPushButton>
#include <QMessageBox>
#include <QLabel>
#include <QHBoxLayout>
#include "../../model/registries/cuttingplanrequestregistry.h"
#include "../viewmodels/request/rowgenerator.h"

bool InputTableManager::_isVerbose = false;


InputTableManager::InputTableManager(QTableWidget* table, QWidget* parent)
    : QObject(parent), _table(table), parent(parent)//,
    //_rowId(table, ColName )
{
}



void InputTableManager::addRow(const Cutting::Plan::Request& request) {
    if (!_table) return;

    int rowIx = _table->rowCount();
    _table->insertRow(rowIx);

    TableRowViewModel vm = Request::ViewModel::RowGenerator::generate(request, this);
    TableRowPopulator::populateRow(_table, rowIx, vm);

    _rowMap.insert(vm.rowId, request);
    _rowIndexMap[vm.rowId] = rowIx;

    if (_isVerbose) {
        zInfo(L("Request row added via generator: %1 | %2")
                  .arg(request.ownerName)
                  .arg(request.externalReference));
    }

}


void InputTableManager::removeRowById(const QUuid& rowId) {

    if (!_table) return;
    if (!_rowIndexMap.contains(rowId)) return;

    int rowIx = _rowIndexMap.value(rowId);

    _table->removeRow(rowIx);


}



void InputTableManager::updateRow(const QUuid& rowId, const Cutting::Plan::Request& request) {
    if (!_table) return;
    if (!_rowIndexMap.contains(rowId)) return;

    int rowIx = _rowIndexMap.value(rowId);
    //_planRowMap[rowId] = instr;
    // updateRow-ban:
    _rowMap.insert(rowId, request);

    // 🔍 Anyag lekérése az azonosító alapján
    const MaterialMaster* mat = nullptr;


    // ViewModel generálása és cellák kirenderelése
    TableRowViewModel vm = Request::ViewModel::RowGenerator::generate(request, this);
    TableRowPopulator::populateRow(_table, rowIx, vm);

    if (_isVerbose) {
        zInfo(L("Request row updated via generator: %1 | %2")
                  .arg(request.ownerName)
                  .arg(request.externalReference));
    }
}

void InputTableManager::refresh_TableFromRegistry() {
    if (!_table)
        return;

    TableUtils::clearSafely(_table);

    const auto& requests = CuttingPlanRequestRegistry::instance().readAll();
    for (const auto& req : requests) {
        addRow(req);  // ✅ feldolgozás és megjelenítés
    }

    //table->resizeColumnsToContents();  // 📐 automatikus oszlopméretezés
}

void InputTableManager::clearTable() {
    if (!_table)
        return;

    _table->setRowCount(0);        // 💣 Teljes sorállomány törlése
    _table->clearContents();       // 🧹 Cellák tartalmának kiürítése (nem kötelező, de biztosra megyünk)
}



