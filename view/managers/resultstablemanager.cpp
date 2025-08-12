#include "resultstablemanager.h"
#include "common/materialutils.h"
#include "common/tableutils/colorlogicutils.h"
#include "model/registries/materialregistry.h"
#include "common/tableutils/resulttable_rowstyler.h"
#include "common/grouputils.h"
#include "common/materialutils.h"
#include <QLabel>
#include <QHBoxLayout>

ResultsTableManager::ResultsTableManager(QTableWidget* table, QWidget* parent)
    : QObject(parent), table(table), parent(parent) {}

void ResultsTableManager::addRow(const QString& rodNumber, const CutPlan& plan) {
    int row = table->rowCount();
    table->insertRow(row);
    table->insertRow(row + 1);  // Badge sor

    // 🧵 Csoportnév badge
    QString groupName = GroupUtils::groupName(plan.materialId);
    QColor groupColor = GroupUtils::colorForGroup(plan.materialId);
    QLabel* groupLabel = new QLabel(groupName.isEmpty() ? "–" : groupName);
    groupLabel->setAlignment(Qt::AlignCenter);
    groupLabel->setStyleSheet(QString(
                                  "QLabel { background-color: %1; color: white; font-weight: bold; padding: 6px; border-radius: 6px; }"
                                  ).arg(groupColor.name()));

    // 🔢 Rod #
    auto* itemRod = new QTableWidgetItem(rodNumber);
    itemRod->setTextAlignment(Qt::AlignCenter);

    // ✂️ Cuts badge-ek
    QWidget* cutsWidget = new QWidget;
    QHBoxLayout* layout = new QHBoxLayout(cutsWidget);
    layout->setContentsMargins(0, 4, 0, 4);
    layout->setSpacing(6);

    for (const Segment& s : plan.segments) {
        QString color;
        switch (s.type) {
        case Segment::Type::Piece:  color = s.length_mm < 300 ? "#e74c3c" : s.length_mm > 2000 ? "#f39c12" : "#27ae60"; break;
        case Segment::Type::Kerf:   color = "#34495e"; break;
        case Segment::Type::Waste:  color = "#bdc3c7"; break;
        }

        QLabel* label = new QLabel(s.toLabelString());
        label->setAlignment(Qt::AlignCenter);
        label->setStyleSheet(QString(
                                 "QLabel { background-color: %1; color: white; font-weight: bold; padding: 3px 8px; border-radius: 6px; }"
                                 ).arg(color));

        if (s.type == Segment::Type::Kerf)
            label->setFixedWidth(60);

        layout->addWidget(label);
    }

    // 📏 Kerf, Waste
    auto* itemKerf = new QTableWidgetItem(QString::number(plan.kerfTotal));
    auto* itemWaste = new QTableWidgetItem(QString::number(plan.waste));
    itemKerf->setTextAlignment(Qt::AlignCenter);
    itemWaste->setTextAlignment(Qt::AlignCenter);

    // 🎨 Háttér waste alapján
    QColor bgColor = plan.waste <= 500 ? QColor(144,238,144)
                     : plan.waste >= 1500 ? QColor(255,120,120)
                                          : QColor(245,245,245);

    for (int col = 0; col < table->columnCount(); ++col) {
        QTableWidgetItem* item = table->item(row, col);
        if (!item) {
            item = new QTableWidgetItem();
            table->setItem(row, col, item);
        }
        item->setBackground(bgColor);
        item->setForeground(Qt::black);
    }

    // 📋 Cellák beállítása
    table->setItem(row, ColRod, itemRod);
    table->setCellWidget(row, ColGroup, groupLabel);
    table->setItem(row, ColKerf, itemKerf);
    table->setItem(row, ColWaste, itemWaste);

    table->setSpan(row + 1, 0, 1, table->columnCount());
    table->setCellWidget(row + 1, 0, cutsWidget);

    const MaterialMaster* mat = MaterialRegistry::instance().findById(plan.materialId);
    ResultTable::RowStyler::applyStyle(table, row, mat, plan);
    ColorLogicUtils::applyBadgeBackground(cutsWidget, MaterialUtils::colorForMaterial(*mat));
}

void ResultsTableManager::clearTable() {
    if (!table) return;
    table->setRowCount(0);
    table->clearContents();
}


