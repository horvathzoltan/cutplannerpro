#include "resultstable_manager.h"
#include "common/logger.h"
#include "view/tablehelpers/tablerowpopulator.h"
#include "view/tableutils/tableutils.h"
#include <QLabel>
#include <QHBoxLayout>
#include "view/viewmodels/result/badgerowgenerator.h"
#include "view/viewmodels/result/rowgenerator.h"

bool ResultsTableManager::_isVerbose = false;


ResultsTableManager::ResultsTableManager(QTableWidget* table, QWidget* parent)
    : QObject(parent), _table(table), parent(parent) {

   TableUtils::applySafeMonospaceFont(table, 11);
}

void ResultsTableManager::addRow(const Cutting::Plan::CutPlan& plan) {
    if (!_table) return;

    int rowIx = _table->rowCount();
    _table->insertRow(rowIx);
    _table->insertRow(rowIx + 1);  // Badge sor

    // Anyag színének meghatározása
    auto vmMain = Results::ViewModel::RowGenerator::generate(plan);
    auto vmBadge = Results::ViewModel::BadgeRowGenerator::generate(plan);

    TableRowPopulator::populateRow(_table, rowIx, vmMain);
    TableRowPopulator::populateRow(_table, rowIx + 1, vmBadge);
    _table->setSpan(rowIx + 1, 0, 1, _table->columnCount());

    if (_isVerbose) {
        zInfo(QString("CuttingPlan row added: %1 | step=%2")
                  .arg(rowIx)
                  .arg(plan.planNumber));
    }
}



void ResultsTableManager::clearTable() {
    if (!_table) return;
    _table->setRowCount(0);
    _table->clearContents();
}


