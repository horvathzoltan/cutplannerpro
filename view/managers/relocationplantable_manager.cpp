#include "relocationplantable_manager.h"
#include "view/cellgenerators/relocationrowviewmodelgenerator.h"
//#include "view/columnindexes/relocationplantable_columns.h"
#include "common/logger.h"
#include "view/tablehelpers/tablerowpopulator.h"

#include <QTableWidgetItem>
#include <QHeaderView>
#include <QBrush>
#include <QColor>

#include <model/registries/materialregistry.h>

bool RelocationPlanTableManager::_isVerbose = false;

/**
 * @brief Konstruktor – inicializálja a táblát és beállítja az oszlopfejléceket.
 */
RelocationPlanTableManager::RelocationPlanTableManager(QTableWidget* table, QWidget* parent)
    : QObject(parent), _table(table), _parent(parent)
{
    if (_table) {
        _table->setColumnCount(6);
        QStringList headers = {"Anyag", "Vonalkód", "Mennyiség", "Forrás", "Cél", "Típus"};
        _table->setHorizontalHeaderLabels(headers);
        _table->horizontalHeader()->setStretchLastSection(true);
    }
}

/**
 * @brief Új sor beszúrása a relokációs terv táblába a generator segítségével.
 *
 * - Meghívja a RelocationRowViewModelGenerator::generate()-t
 * - A kapott TableRowViewModel celláit beírja a QTableWidget-be
 * - A rowId-t eltárolja a belső map-ekben
 */
void RelocationPlanTableManager::addRow(const RelocationInstruction& instr) {
    if (!_table) return;

    int rowIx = _table->rowCount();
    _table->insertRow(rowIx);

    // 🔍 Anyag lekérése az azonosító alapján
    const MaterialMaster* mat = nullptr;
    // if(instr.sourceType == AuditSourceType::Stock){
    //      mat = MaterialRegistry::instance().findByBarcode(instr.barcode);
    // } else {
    //     std::optional<LeftoverStockEntry> a =
    //         LeftoverStockRegistry::instance().findByBarcode(instr.barcode);
    //     if(a.has_value())
    //         mat = MaterialRegistry::instance().findById(a->materialId);
    // }
    mat = MaterialRegistry::instance().findById(instr.materialId);
    if (!mat)
        return;

    // ViewModel generálása és cellák kirenderelése
    TableRowViewModel vm = RelocationRowViewModelGenerator::generate(instr, mat);
    TableRowPopulator::populateRow(_table, rowIx, vm);

    // rowId mentése
    _planRowMap[vm.rowId] = instr;
    _rowIndexMap[vm.rowId] = rowIx;

    if (_isVerbose) {
        zInfo(L("RelocationPlan row added via generator: %1 | %2")
                  .arg(instr.materialCode)
                  .arg(instr.barcode));
    }
}

/**
 * @brief Meglévő sor frissítése rowId alapján.
 *
 * - Újra legenerálja a ViewModel-t a frissített RelocationInstruction-ből
 * - Frissíti a cellák tartalmát a táblában
 */
void RelocationPlanTableManager::updateRow(const QUuid& rowId, const RelocationInstruction& instr) {
    if (!_table) return;
    if (!_rowIndexMap.contains(rowId)) return;

    int rowIx = _rowIndexMap.value(rowId);
    _planRowMap[rowId] = instr;

    // 🔍 Anyag lekérése az azonosító alapján
    const MaterialMaster* mat = nullptr;
    // if(instr.sourceType == AuditSourceType::Stock){
    //     mat = MaterialRegistry::instance().findByBarcode(instr.barcode);
    // } else {
    //     std::optional<LeftoverStockEntry> a =
    //         LeftoverStockRegistry::instance().findByBarcode(instr.barcode);
    //     if(a.has_value())
    //         mat = MaterialRegistry::instance().findById(a->materialId);
    // }
    mat = MaterialRegistry::instance().findById(instr.materialId);
    if (!mat)
        return;

    // ViewModel generálása és cellák kirenderelése
    TableRowViewModel vm = RelocationRowViewModelGenerator::generate(instr, mat);
    TableRowPopulator::populateRow(_table, rowIx, vm);

    if (_isVerbose) {
        zInfo(L("RelocationPlan row updated via generator: %1 | %2")
                  .arg(instr.materialCode)
                  .arg(instr.barcode));
    }
}

/**
 * @brief Teljes tábla törlése.
 */
void RelocationPlanTableManager::clearTable() {
    if (!_table) return;

    _table->clearContents();
    _table->setRowCount(0);
    _planRowMap.clear();
    _rowIndexMap.clear();

    if (_isVerbose) {
        zInfo(L("RelocationPlan table cleared"));
    }
}
