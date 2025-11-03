#include "resultstable_manager.h"
#include "common/logger.h"
//#include "model/material/materialgroup_utils.h"
//#include "view/cellhelpers/auditcelltooltips.h"
#include "view/tablehelpers/tablerowpopulator.h"
//#include "view/tableutils/colorlogicutils.h"
//#include "model/registries/materialregistry.h"
//#include "view/tableutils/resulttable_rowstyler.h"
//#include "model/material/materialgroup_utils.h"
//#include "model/material/material_utils.h"
#include "view/tableutils/tableutils.h"
#include <QLabel>
#include <QHBoxLayout>
//#include "view/columnindexes/tableresults_columns.h"
#include "view/viewmodels/result/badgerowgenerator.h"
#include "view/viewmodels/result/rowgenerator.h"

bool ResultsTableManager::_isVerbose = false;


ResultsTableManager::ResultsTableManager(QTableWidget* table, QWidget* parent)
    : QObject(parent), _table(table), parent(parent) {

   TableUtils::applySafeMonospaceFont(table, 10);
}

void ResultsTableManager::addRow(const QString& rodNumber,
                                 const Cutting::Plan::CutPlan& plan) {
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

// void ResultsTableManager::addRow_old(const QString& rodNumber, const Cutting::Plan::CutPlan& plan) {
//     int row = _table->rowCount();
//     _table->insertRow(row);
//     _table->insertRow(row + 1);  // Badge sor

//     // 🧵 Csoportnév badge
//     QString groupName = GroupUtils::groupName(plan.materialId);
//     QColor groupColor = GroupUtils::colorForGroup(plan.materialId);

//     // Anyagnév a registryből
//     QString materialName = plan.materialName();

//     // Ha van kategória, akkor "Category + Anyagnév", ha nincs, akkor csak Anyagnév
//     QString categoryText = groupName.isEmpty()
//                                ? materialName
//                                : groupName + " | " + materialName;

//     QLabel* categoryLabel = new QLabel(categoryText);
//     categoryLabel->setAlignment(Qt::AlignCenter);
//     categoryLabel->setStyleSheet(QString(
//                                   "QLabel { background-color: %1; color: white; font-weight: bold; padding: 6px; border-radius: 6px; }"
//                                   ).arg(groupColor.name()));

//     // 🔢 Rod #
//     // Globális planNumber + RodNumber + Barcode
//     QString rodLabel = QString("%1|%2").arg(plan.rodId, plan.sourceBarcode);
//     auto* itemRod = new QTableWidgetItem(rodLabel);
//     QString rod_tooltip_txt = QString("RodId: %1\n"
//                              "Barcode: %2\n"
//                              "Parent: %3\n"
//                              "Forrás: %4\n"
//                              "OptimizationId: %5")
//                          .arg(plan.rodId)
//                          .arg(plan.sourceBarcode.isEmpty() ? "—" : plan.sourceBarcode)
//                          .arg(plan.parentBarcode.value_or("—"))
//                          .arg(plan.source == Cutting::Plan::Source::Stock ? "Stock"
//                               : plan.source == Cutting::Plan::Source::Reusable ? "Reusable"
//                                                                                : "Optimization")
//                          .arg(plan.optimizationId);

//     itemRod->setToolTip(rod_tooltip_txt);

//     itemRod->setTextAlignment(Qt::AlignCenter);

//     // ✂️ Cuts badge-ek
//     QWidget* cutsWidget = new QWidget;
//     QHBoxLayout* layout = new QHBoxLayout(cutsWidget);
//     layout->setContentsMargins(0, 4, 0, 4);
//     layout->setSpacing(6);

//     for (const Cutting::Segment::SegmentModel& s : plan.segments) {
//         QString color;
//         switch (s.type()) {
//         case Cutting::Segment::SegmentModel::Type::Piece:  color = s.length_mm() < 300 ? "#e74c3c" : s.length_mm() > 2000 ? "#f39c12" : "#27ae60"; break;
//         case Cutting::Segment::SegmentModel::Type::Kerf:   color = "#34495e"; break;
//         case Cutting::Segment::SegmentModel::Type::Waste:  color = "#bdc3c7"; break;
//         }

//         // Badge szegmens label
//         QString badgeLabel= s.toLabelString();

//         // Tooltip: részletes infó
//         // Tooltip: részletes infó (audit-barát)
//         // QString badge_tooltip_txt = QString(
//         //                                 "RodId: %1\n"
//         //                                 "Barcode: %2\n"
//         //                                 "Material: %3\n"
//         //                                 "Csoport: %4\n"
//         //                                 "CutSize: %5 mm\n"
//         //                                 "Remaining: %6 mm\n"
//         //                                 "Parent: %7\n"
//         //                                 "Forrás: %8\n"
//         //                                 "OptimizationId: %9\n"
//         //                                 "Gép: %10\n"
//         //                                 "Státusz: %11")
//         //                                 .arg(plan.rodId)
//         //                                 .arg(s.barcode())
//         //                                 .arg(plan.materialName())
//         //                                 .arg(groupName.isEmpty() ? "Nincs csoport" : groupName)
//         //                                 .arg(s.length_mm())
//         //                                 .arg(s.type() == Cutting::Segment::SegmentModel::Type::Waste ? QString::number(s.length_mm()) : "—")
//         //                                 .arg(plan.parentBarcode.value_or("—"))
//         //                                 .arg(plan.source == Cutting::Plan::Source::Stock ? "Stock"
//         //                                      : plan.source == Cutting::Plan::Source::Reusable ? "Reusable"
//         //                                                                                       : "Optimization")
//         //                                 .arg(plan.optimizationId)
//         //                                 .arg(plan.machineName)
//         //                                 .arg(Cutting::Plan::statusText(plan.status));

//         QString badge_tooltip_txt = AuditCellTooltips::forSegment(s, plan, groupName);

//         zInfo("ResultsTableManager::addRow badgeLabel:"+badgeLabel);
//         zInfo("ResultsTableManager::addRow badge_tooltip_txt:"+badge_tooltip_txt);

//         QLabel* label = new QLabel(badgeLabel);
//         label->setToolTip(badge_tooltip_txt);

//         label->setAlignment(Qt::AlignCenter);
//         label->setStyleSheet(QString(
//                                  "QLabel { background-color: %1; color: white; font-weight: bold; padding: 3px 8px; border-radius: 6px; }"
//                                  ).arg(color));

//         if (s.type() == Cutting::Segment::SegmentModel::Type::Kerf)
//             label->setFixedWidth(60);

//         layout->addWidget(label);
//     }

//     // 📏 Length
//     auto* itemLength = new QTableWidgetItem(QString::number(plan.totalLength));
//     itemLength->setTextAlignment(Qt::AlignCenter);

//     // kerf
//     auto* itemKerf = new QTableWidgetItem(QString::number(plan.kerfTotal));
//     itemKerf->setTextAlignment(Qt::AlignCenter);

//     // Csak a legutolsó waste szegmens
//     // 📏 Waste
//     auto* itemWaste = new QTableWidgetItem(QString::number(plan.waste));
//     itemWaste->setTextAlignment(Qt::AlignCenter);
//     _table->setItem(row, ResultsTableColumns::Waste, itemWaste);

//     // 📋 Cellák beállítása
//     _table->setItem(row, ResultsTableColumns::RodId, itemRod);
//     _table->setCellWidget(row, ResultsTableColumns::Category, categoryLabel);
//     _table->setItem(row, ResultsTableColumns::Length, itemLength);
//     _table->setItem(row, ResultsTableColumns::Kerf, itemKerf);

//     _table->setSpan(row + 1, 0, 1, _table->columnCount());
//     _table->setCellWidget(row + 1, 0, cutsWidget);

//     const MaterialMaster* mat = MaterialRegistry::instance().findById(plan.materialId);

//     // csak colort állít be
//     ResultTable::RowStyler::applyStyle(_table, row, mat, plan);
//     // csak stylesheetet állít be
//     ColorLogicUtils::applyBadgeBackground(cutsWidget, MaterialUtils::colorForMaterial(*mat));
// }

void ResultsTableManager::clearTable() {
    if (!_table) return;
    _table->setRowCount(0);
    _table->clearContents();
}


