#include "resultstable_manager.h"
#include "model/material/materialgroup_utils.h"
#include "view/tableutils/colorlogicutils.h"
#include "model/registries/materialregistry.h"
#include "view/tableutils/resulttable_rowstyler.h"
#include "model/material/materialgroup_utils.h"
#include "model/material/material_utils.h"
#include "view/tableutils/tableutils.h"
#include <QLabel>
#include <QHBoxLayout>

ResultsTableManager::ResultsTableManager(QTableWidget* table, QWidget* parent)
    : QObject(parent), table(table), parent(parent) {

   TableUtils::applySafeMonospaceFont(table, 10);
}

QString ResultsTableManager::formatWasteBadge(const Cutting::Plan::CutPlan& plan, int wasteIndex) {
    QString badge;
    if (plan.leftoverBarcode.isEmpty()) {
        badge = QString("[%1|%2|W%3]")
        .arg(plan.rodId)                        // 🔑 Stabil rúd azonosító
            .arg(IdentifierUtils::unidentified())   // ⬅ explicit UNIDENTIFIED
            .arg(wasteIndex);
    } else {
        badge = QString("[%1|%2|W%3]")
        .arg(plan.rodId)
            .arg(plan.leftoverBarcode)
            .arg(wasteIndex);
    }
    return badge;
}


void ResultsTableManager::addRow(const QString& rodNumber, const Cutting::Plan::CutPlan& plan) {
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

    // auto* itemLength = new QTableWidgetItem(QString::number(mat->stockLength_mm));
    // itemLength->setTextAlignment(Qt::AlignCenter);
    // //itemLength->setData(Qt::UserRole, mat->stockLength_mm);
    // table->setItem(rowIx, ColLength, itemLength);

    // 🔢 Rod #
    // Globális planNumber + RodNumber + Barcode
    QString rodLabel = QString("Rod %1").arg(plan.sourceBarcode.isEmpty() ? plan.rodId : plan.sourceBarcode);

    auto* itemRod = new QTableWidgetItem(rodLabel);

    // Tooltipben mindkettő: konkrét rodId és materialBarcode
    itemRod->setToolTip(QString("RodId: %1\nBarcode: %2\nMaterial: %3")
                            .arg(plan.rodId.isEmpty() ? "—" : plan.rodId)
                            .arg(plan.sourceBarcode.isEmpty() ? "—" : plan.sourceBarcode)
                            .arg(plan.materialBarcode()));


    itemRod->setTextAlignment(Qt::AlignCenter);

    // ✂️ Cuts badge-ek
    QWidget* cutsWidget = new QWidget;
    QHBoxLayout* layout = new QHBoxLayout(cutsWidget);
    layout->setContentsMargins(0, 4, 0, 4);
    layout->setSpacing(6);

    for (const Cutting::Segment::SegmentModel& s : plan.segments) {
        QString color;
        switch (s.type) {
        case Cutting::Segment::SegmentModel::Type::Piece:  color = s.length_mm < 300 ? "#e74c3c" : s.length_mm > 2000 ? "#f39c12" : "#27ae60"; break;
        case Cutting::Segment::SegmentModel::Type::Kerf:   color = "#34495e"; break;
        case Cutting::Segment::SegmentModel::Type::Waste:  color = "#bdc3c7"; break;
        }

        QString segBarcode;

        // Ha a szegmensnek van saját barcode-ja, azt használjuk
        if (!s.barcode.isEmpty() && s.barcode != "UNIDENTIFIED") {
            segBarcode = s.barcode;
        }
        // Ha waste szegmens és a plan shortcut is ismert, azt használjuk
        else if (s.type == Cutting::Segment::SegmentModel::Type::Waste && !plan.leftoverBarcode.isEmpty()) {
            segBarcode = plan.leftoverBarcode;
        }
        // Egyébként rodId vagy materialBarcode
        else {
            segBarcode = plan.rodId.isEmpty() ? plan.materialBarcode() : plan.rodId;
        }

        QLabel* label = new QLabel(
            s.toLabelString(QString("Rod %1").arg(plan.rodId), segBarcode)
            );

        // Tooltip: részletes infó
        label->setToolTip(QString("Rod: %1\nBarcode: %2\nMaterial: %3")
                              .arg(plan.rodId)   // 🔑 Stabil rúd azonosító
                              .arg(plan.rodId.isEmpty() ? "—" : plan.rodId)
                              .arg(plan.materialBarcode()));

        label->setAlignment(Qt::AlignCenter);
        label->setStyleSheet(QString(
                                 "QLabel { background-color: %1; color: white; font-weight: bold; padding: 3px 8px; border-radius: 6px; }"
                                 ).arg(color));

        if (s.type == Cutting::Segment::SegmentModel::Type::Kerf)
            label->setFixedWidth(60);

        layout->addWidget(label);
    }

    // 📏 Kerf, Waste
    auto* itemKerf = new QTableWidgetItem(QString::number(plan.kerfTotal));
    auto* itemLength = new QTableWidgetItem(QString::number(plan.totalLength));

    //int wasteCounter = 0;
    // Csak a legutolsó waste szegmens
    QString wasteBadge;
    auto it = std::find_if(plan.segments.rbegin(), plan.segments.rend(),
                           [](const auto& s){ return s.type == Cutting::Segment::SegmentModel::Type::Waste; });

    if (it != plan.segments.rend()) {
        // mindig csak az utolsó waste szegmens
        wasteBadge = formatWasteBadge(plan, 1);
    }

    auto* itemWaste = new QTableWidgetItem(wasteBadge.isEmpty()
                                               ? QString::number(plan.waste)
                                               : wasteBadge);
    itemWaste->setTextAlignment(Qt::AlignCenter);
    table->setItem(row, ColWaste, itemWaste);


    itemKerf->setTextAlignment(Qt::AlignCenter);
    itemLength->setTextAlignment(Qt::AlignCenter);

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
    table->setItem(row, ColLength, itemLength);
    table->setItem(row, ColKerf, itemKerf);

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


