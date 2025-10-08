#include "relocationplantable_manager.h"
#include "view/viewmodels/relocation/rowgenerator.h"
//#include "view/columnindexes/relocationplantable_columns.h"
#include "common/logger.h"

#include "view/dialog/relocation/relocationquantitydialog.h"
#include "view/tablehelpers/relocationquantityhelpers.h"

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

        // connect(_table, &QTableWidget::cellClicked,
        //         this, &RelocationPlanTableManager::onCellClicked);

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
    TableRowViewModel vm = Relocation::ViewModel::RowGenerator::generate(instr, mat, this);
    TableRowPopulator::populateRow(_table, rowIx, vm);

    // rowId mentése
    //_planRowMap[vm.rowId] = instr;
    _planRowMap.insert(vm.rowId, instr);
    _rowIndexMap[vm.rowId] = rowIx;


    if (_isVerbose) {
        zInfo(L("RelocationPlan row added via generator: %1 | %2")
                  .arg(instr.materialName)
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
    //_planRowMap[rowId] = instr;
    // updateRow-ban:
    _planRowMap.insert(rowId, instr);

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
    TableRowViewModel vm = Relocation::ViewModel::RowGenerator::generate(instr, mat, this);
    TableRowPopulator::populateRow(_table, rowIx, vm);

    if (_isVerbose) {
        zInfo(L("RelocationPlan row updated via generator: %1 | %2")
                  .arg(instr.materialName)
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

void RelocationPlanTableManager::editRow(const QUuid& rowId, const QString& mode) {
    auto it = _planRowMap.find(rowId);
    if (it == _planRowMap.end())
        return;

    RelocationInstruction& instruction = it.value();

    zInfo(L("editRow: sources=%1 targets=%2")
        .arg(instruction.sources.size())
        .arg(instruction.targets.size()));


    // 🔹 Dialógus előkészítése
    RelocationQuantityDialog dlg(_parent);
    if (mode == "source") {
        dlg.setMode(QuantityDialogMode::Source);
        auto rows = RelocationQuantityHelpers::generateSourceRows(instruction);
        dlg.setRows(rows, instruction.plannedQuantity, -1);
    }
    else if (mode == "target") {
        dlg.setMode(QuantityDialogMode::Target);
        auto rows = RelocationQuantityHelpers::generateTargetRows(instruction);
        int totalMoved = 0;
        for (const auto& src : instruction.sources)
            totalMoved += src.moved;

        dlg.setRows(rows, instruction.plannedQuantity, totalMoved);    }

    // Fejléc beállítása a mode alapján
    if (mode == "source") {
        dlg.setWindowTitle(tr("Forrás tárhelyek szerkesztése"));
        auto rows = RelocationQuantityHelpers::generateSourceRows(instruction);
        dlg.setRows(rows, instruction.plannedQuantity, -1);
        zInfo(L("generateSourceRows: %1 sor").arg(rows.size()));

    } else if (mode == "target") {
        dlg.setWindowTitle(tr("Cél tárhelyek szerkesztése"));
        auto rows = RelocationQuantityHelpers::generateTargetRows(instruction);
        int totalMoved = 0;
        for (const auto& src : instruction.sources)
            totalMoved += src.moved;

        dlg.setRows(rows, instruction.plannedQuantity, totalMoved);
        zInfo(L("generateTargetRows: %1 sor").arg(rows.size()));
    } else {
        // fallback: teljes nézet
        dlg.setWindowTitle(tr("Relokációs mennyiségek szerkesztése"));
        auto rows = RelocationQuantityHelpers::generateQuantityRows(instruction);
        dlg.setRows(rows, instruction.plannedQuantity, -1);
        zInfo(L("generateQuantityRows: %1 sor").arg(rows.size()));
    }


    if (dlg.exec() == QDialog::Accepted) {
        QVector<RelocationQuantityRow> result = dlg.getRows();

        if (mode == "source") {
            RelocationQuantityHelpers::applySourceRows(instruction, result);
        } else if (mode == "target") {
            RelocationQuantityHelpers::applyTargetRows(instruction, result);
        } else {
            RelocationQuantityHelpers::applyQuantityRows(instruction, result);
        }

        updateRow(rowId, instruction); // frissítés a táblában
    }
}

void RelocationPlanTableManager::finalizeRow(const QUuid& rowId) {
    if (!_planRowMap.contains(rowId)) return;

    RelocationInstruction& instr = _planRowMap[rowId];
    if (!instr.isReadyToFinalize() || instr.isAlreadyFinalized()) return;

    instr.executedQuantity = instr.plannedQuantity;
    instr.isFinalized = true;

    updateRow(rowId, instr); // újragenerálja a sort
}

